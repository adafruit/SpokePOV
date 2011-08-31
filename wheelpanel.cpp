#include <wx/frame.h>
#include <wx/log.h>
#include <wx/debug.h>
#include <wx/event.h>
#include <wx/dcclient.h>
#include <wx/brush.h>
#include <wx/dcbuffer.h>
#include <wx/math.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/image.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include "wheelpanel.h"
#include "globals.h"

WheelPanel::WheelPanel(wxWindow *parent, wxPanel *ctl, int num_leds, int hub_size) :
  wxPanel(parent) {
  
  this->hubsize = hub_size;
  this->num_leds = num_leds;

  origimg = NULL;

  controlpanel = ctl;

  model = new bool *[ROWS_PER_WHEEL];
  for (int i=0; i<ROWS_PER_WHEEL; i++) {
    model[i] = new bool[30];
    for (int j=0; j<30; j++) 
      model[i][j] = false;
  }

  savedname = wxString("");
  controlsizer = new wxBoxSizer(wxHORIZONTAL);
  controlpanel->SetSizer(controlsizer);
  //controlsizer->SetSizeHints(this);
  Read = new wxButton(controlpanel, BUTTON_ID_READ, "Read from SpokePOV");
  Read->Hide();
  Write = new wxButton(controlpanel, BUTTON_ID_WRITE, "Write to SpokePOV");
  Write->Hide();
  Verify = new wxButton(controlpanel, BUTTON_ID_VERIFY, "Verify SpokePOV")
;
  Verify->Hide();

  Done = new wxButton(controlpanel, BUTTON_ID_DONE, "Done adjusting import image");
  Done->Hide();
  Donetext = new wxStaticText(controlpanel, wxID_ANY, "You can now resize (drag the corner boxes) or move (use the 'hand' tool) the image around, when you're done, click here: ");
  Donetext->Hide();

  controlsizer->Add(Read, 1, wxEXPAND|wxALL|wxALIGN_LEFT|wxFIXED_MINSIZE, 15); 
  controlsizer->Add(Write, 1, wxEXPAND|wxALL|wxFIXED_MINSIZE, 15);
  controlsizer->Add(Verify, 1, wxEXPAND|wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 15); 
  controlsizer->Add(Donetext, 1, wxEXPAND|wxALL|wxALIGN_LEFT, 10);
  controlsizer->Add(Done, 0, wxEXPAND|wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);

 setupRWVcontrols();
}


void WheelPanel::setupRWVcontrols(void) {
  wxSetCursor(*wxSTANDARD_CURSOR);

  //controlsizer->Detach(Done);
  Done->Hide();
  //controlsizer->Detach(Donetext);
  Donetext->Hide();

  Read->Show();
  Write->Show();
  Verify->Show();

  controlsizer->Layout();

  drawstate = NONE;
  Refresh();
}

void WheelPanel::setupImageControl(void) {
  Read->Hide();
  Write->Hide();
  Verify->Hide();

  Donetext->Show();
  Done->Show();

  controlsizer->Layout();
}

void WheelPanel::OnPaint(wxPaintEvent& event) {
  wxBufferedPaintDC dc(this);
  Draw(&dc);
}


bool WheelPanel::GetModel(unsigned char buff[]) {
  int t;
  for (int i=0; i<ROWS_PER_WHEEL; i++) {
    for (int j = 0; j<4; j++) {
      t = 0;
      for (int k=0; k<8; k++) {
	t <<= 1;
	if (! model[i][j*8+k]) {
	  t |= 0x1;
	}
      }
      buff[i*4+j] = t;
    }
  }
  return true;
}


bool WheelPanel::SetModel(unsigned char buff[1024]) {
  int row, led;
  for (int i=0; i<1024; i++) {
      row = i/4;
      for (int j = 0; j<8; j++) {
	led = (i%4)*8+j;
	if (buff[i] & (1<<(7-j)))
	  model[row][led] = false;
	else
	  model[row][led] = true;
      }   
  }
  Refresh();
  return true;
}

bool WheelPanel::LoadModel(wxString name) {
  wxInputStream *fis = new wxFileInputStream(name);
  if (fis->IsOk()) {
    wxDataInputStream *dis = new wxDataInputStream(*fis);
    int len = dis->Read32();
    if (len != 8) {
      wxMessageBox("Not a valid SpokePOV file!", "Invalid file", wxICON_HAND|wxOK);
      return false;
    }
    fis->SeekI(0);
    dis = new wxDataInputStream(*fis);
    if (dis->ReadString() != "SpokePOV") { // identifier
      wxMessageBox("Not a valid SpokePOV file!", "Invalid file", wxICON_HAND|wxOK);
      return false;
    }
    int maj=dis->Read8();
    int min=dis->Read8();
    if ((maj != 1) &&
	(min != 2)) {
      wxMessageBox(wxString::Format("Found a file with version \"%d.%d\" which is not supported!", maj, min), "Wrong version",wxICON_HAND|wxOK);
      return false;
    }
    
    int leds = dis->Read8(); // basically the size
    if (leds != num_leds) {
      wxMessageBox(wxString::Format("This file has %d linear pixels (LEDs), while this SpokePOV only supports %d leds", leds, num_leds), "File mismatch", wxICON_HAND|wxOK );
      return false;
    }

    int pix = dis->Read16();
    if (pix != ROWS_PER_WHEEL) {
      wxMessageBox(wxString::Format("This file has %d radial pixels, while this SpokePOV only supports %d pixels", pix, ROWS_PER_WHEEL), "File mismatch", wxICON_HAND|wxOK);
      return false;
    }

    for (int led = 0; led < num_leds; led++) {
      for (int row = 0; row < ROWS_PER_WHEEL; row++) {
	//wxLogDebug("%d %d", led, row);
	if (dis->Read8() == 0) {
	  model[row][led] = false;
	} else {
	  model[row][led] = true;
	}
      }
    }
    if (origimg)
      delete origimg;
    origimg = NULL;
    delete dis;
    delete fis;
    Refresh();
    savedname = name;
    return true;
  }
  delete fis;
  return false;
}

bool WheelPanel::ImportBMP(wxString name) {
  if (origimg)
    delete origimg;
  origimg = new wxImage();
  if (! origimg->LoadFile(name))
    return false;
  wxLogDebug("%d %d", origimg->GetWidth(), origimg->GetHeight());
  origimg->ConvertToMono(0,0,0);
  currimgpos = wxPoint(0,0);
  currimg = origimg->Copy();
  Refresh();

  setupImageControl();

  drawstate = IMAGEFOLLOW;
  return true;
}

bool WheelPanel::SaveModel(void) {
  wxOutputStream *fos = new wxFileOutputStream(savedname);
  if (fos->IsOk()) {
    wxDataOutputStream *dos = new wxDataOutputStream(*fos);

    dos->WriteString("SpokePOV"); // identifier

    dos->Write8(1); // version 1.2
    dos->Write8(2);

    dos->Write8(30); // basically the size
    dos->Write16(ROWS_PER_WHEEL); 

    for (int led = 0; led < num_leds; led++) {
      for (int row = 0; row < ROWS_PER_WHEEL; row++) {
	//wxLogDebug("%d %d", led, row);
	if (model[row][led])
	  dos->Write8(0xff);
	else
	  dos->Write8(0);
      }
    }
    delete fos;
    return true;
  }
  delete fos;
  return false;
}

void WheelPanel::Draw(wxDC *dc) {
  //  wxLogDebug("draw");
  wxSize size = GetClientSize();

  // clear background
  //wxLogDebug("size: %d %d", size.x, size.y);
  dc->Clear();
  dc->SetBrush(*wxWHITE_BRUSH);
  dc->DrawRectangle(0, 0, size.x, size.y);

  if ((drawstate == IMAGEFOLLOW) || (drawstate == IMAGEMOVE)
      || (drawstate == IMAGERESIZE_SE) || (drawstate == IMAGERESIZE_NE)
      || (drawstate == IMAGERESIZE_SW) || (drawstate == IMAGERESIZE_NW)
      ) {
    wxPoint p;
    for (int row=0; row < ROWS_PER_WHEEL; row++) {
      for (int led=0; led<num_leds; led++) {
	p = getXYPointForLED(led, row);
	p -= currimgpos;
	//wxLogDebug("Led %d Row %d xy: (%d, %d)", led, row, p.x, p.y);
	if ((p.x >= currimg.GetWidth()) || (p.x < 0) ||
	    (p.y >= currimg.GetHeight()) || (p.y < 0))
	  model[row][30-num_leds+led] = 0;
	else {
	  if (currimg.GetRed(p.x, p.y) == 0) {
	    model[row][30-num_leds+led] = 0xFF;
	  } else {
	    model[row][30-num_leds+led] = 0;
	  }
	}
      }
    }
  }

  // draw background image
  if (origimg != NULL) {
    dc->DrawBitmap(wxBitmap(currimg,1), currimgpos.x, currimgpos.y);
    //wxLogDebug("Drawing at %d,%d", currimgpos.x, currimgpos.y);
    if ((drawstate == IMAGEFOLLOW) || (drawstate == IMAGEMOVE)
	|| (drawstate == IMAGERESIZE_SE) || (drawstate == IMAGERESIZE_NE)
	|| (drawstate == IMAGERESIZE_SW) || (drawstate == IMAGERESIZE_NW)) {
      // draw bounding box
      wxSize imgsize = wxSize(currimg.GetWidth(), currimg.GetHeight());
      wxSize tabsize = wxSize(TABSIZE,TABSIZE);
      dc->SetPen(wxPen(*wxBLACK, 1, wxSHORT_DASH));
      dc->SetBrush(*wxTRANSPARENT_BRUSH);
      dc->DrawRectangle(currimgpos, imgsize);
      
      // draw tabs
      dc->SetPen(wxPen(*wxBLACK, 1, wxSOLID));
      dc->SetBrush(*wxBLACK_BRUSH);
      dc->DrawRectangle(currimgpos, tabsize);
      dc->DrawRectangle(currimgpos+imgsize-tabsize, tabsize);
      dc->DrawRectangle(currimgpos+wxSize(0, currimg.GetHeight()-TABSIZE), tabsize);
      dc->DrawRectangle(currimgpos+wxSize(currimg.GetWidth()-TABSIZE, 0), tabsize);
    }
  }
  // setup variables
  wxPoint middle = wxPoint(size.x/2, size.y/2);
  float dDiameter = (size.x < size.y) ? size.x : size.y;
  dDiameter /= (num_leds + hubsize);

  // draw rings
  dc->SetPen(*wxGREY_PEN);
  dc->SetBrush(*wxTRANSPARENT_BRUSH);
  for (int i=0; i<num_leds+1; i++) {
    float diam = dDiameter * (hubsize+i);
    dc->DrawCircle(middle.x, middle.y, (int)(diam/2));
  }
  
  // draw spokes
  float innerringD = dDiameter*hubsize;
  float outerringD = dDiameter*(num_leds+hubsize);
  for (int rad=0; rad<ROWS_PER_WHEEL; rad++) {
    float x1 = innerringD/2.0 * sin(2*PI * rad / (float)ROWS_PER_WHEEL);
    float x2 = outerringD/2.0 * sin(2*PI * rad / (float)ROWS_PER_WHEEL);
    float y1 = innerringD/2.0 * cos(2*PI * rad / (float)ROWS_PER_WHEEL);
    float y2 = outerringD/2.0 * cos(2*PI * rad / (float)ROWS_PER_WHEEL);
    dc->DrawLine(middle.x+(int)x1, middle.y+(int)y1, middle.x+(int)x2, middle.y+(int)y2);
  }

  // draw model
  wxPen *pen = new wxPen(*wxRED, (int)(dDiameter/2), wxSOLID);
  pen->SetCap(wxCAP_BUTT);
  dc->SetPen(*pen);
  /*
  float dAngle = 360.0/ROWS_PER_WHEEL;
  for (int led = 0; led < num_leds; led++) {
    for (int row = 0; row < ROWS_PER_WHEEL; row++) {
      //wxLogDebug("%d %d", led, row);
      if (model[row][30-num_leds+led]) {
	float startangle = 90.0 - ((row+1)*360.0/ROWS_PER_WHEEL);
	float diam = (led+.5+hubsize) * dDiameter;
	dc->DrawEllipticArc(int(size.x/2 - (diam/2) + .5),
			    int(size.y/2 - (diam/2) + .5), 
			    int(diam), int(diam),
			    startangle,startangle+dAngle);
      }
    }
  }
  */
  for (int led = 0; led < num_leds; led++) {
    int row = 0;
    while (row < ROWS_PER_WHEEL) {
      while ((row < ROWS_PER_WHEEL) &&
	     (model[row++][30-num_leds+led] == 0));

      if (row == ROWS_PER_WHEEL)
	continue;

      // ok time to mark

      int start = row-1;
      while ((row < ROWS_PER_WHEEL) &&
	     (model[row++][30-num_leds+led] != 0));
      int end = row-1;
      float startangle = 90.0 - ((start)*360.0/(ROWS_PER_WHEEL-1));
      float endangle = 90.0 - ((end)*360.0/(ROWS_PER_WHEEL-1));
      float diam = (led+.5+hubsize) * dDiameter;
      dc->DrawEllipticArc(int(size.x/2 - (diam/2) + .5),
			  int(size.y/2 - (diam/2) + .5), 
			  int(diam), int(diam),
			  endangle, startangle);
      //wxLogDebug("LED %d start %f end %f", led, startangle, endangle);
    }
  }
}

void WheelPanel::OnChar(wxKeyEvent &event) {
  shiftdown = event.ShiftDown();
  event.Skip();
}

void WheelPanel::OnLeftUp(wxMouseEvent &event) {
  if (HasCapture())
    ReleaseMouse();
  
  switch (drawstate) {
  case DRAWING:
  case ERASING:
    drawstate = NONE;
    break;
  case IMAGEFOLLOW:
    break;
  case IMAGEMOVE:
  case IMAGERESIZE_SE:
  case IMAGERESIZE_NE:
  case IMAGERESIZE_SW:
  case IMAGERESIZE_NW:
    drawstate = IMAGEFOLLOW;
    break;
  }
}

void WheelPanel::OnLeftDown(wxMouseEvent &event) {
  CaptureMouse();
  SetFocus();

  wxPoint pos = event.GetPosition();
    
  switch (drawstate) {
  case IMAGEFOLLOW: {
    // in the square?
    if (! origimg) 
      return;
    if ((pos.x >= currimgpos.x+10) && (pos.x <= currimgpos.x+currimg.GetWidth()-10) &&
	(pos.y >= currimgpos.y+10) && (pos.y <= currimgpos.y+currimg.GetHeight()-10)) {
      // start dragging!
      drawstate = IMAGEMOVE;
      imagemovepos = pos;
      origimgpos = currimgpos;
      //wxLogDebug("dragging from (%d, %d)", pos.x, pos.y);
    } else if ((pos.x > currimgpos.x+currimg.GetWidth()-10) && 
	       (pos.x < currimgpos.x+currimg.GetWidth()) &&
	       (pos.y > currimgpos.y+currimg.GetHeight()-10) && 
	       (pos.y < currimgpos.y+currimg.GetHeight())) {
      // in the south east corner
      wxLogDebug("Resizing SE");
      drawstate = IMAGERESIZE_SE;
      imageresizepos = pos;
      lastsize = wxSize(currimg.GetWidth(), currimg.GetHeight());
    } else if  ((pos.x >= currimgpos.x) && (pos.x < currimgpos.x+10) &&
		(pos.y >= currimgpos.y) && (pos.y < currimgpos.y+10)) {
      // north west
      wxLogDebug("Resizing NW");
      drawstate = IMAGERESIZE_NW;
      origimgpos = currimgpos;
      imagemovepos = imageresizepos = pos;
      lastsize = wxSize(currimg.GetWidth(), currimg.GetHeight());
    } else if ((pos.x >= currimgpos.x+currimg.GetWidth()-10) && 
	       (pos.x < currimgpos.x+currimg.GetWidth()) &&
	       (pos.y >= currimgpos.y) && (pos.y < currimgpos.y+10)) {
      // north east
      wxLogDebug("Resizing NE");
      drawstate = IMAGERESIZE_NE;
      origimgpos = currimgpos;
      imagemovepos = imageresizepos = pos;
      lastsize = wxSize(currimg.GetWidth(), currimg.GetHeight());
    } else if ((pos.x > currimgpos.x) && (pos.x < currimgpos.x+10) &&
	       (pos.y > currimgpos.y+currimg.GetHeight()-10) && 
	       (pos.y < currimgpos.y+currimg.GetHeight())) {
      // south west
      wxLogDebug("Resizing SW");
      drawstate = IMAGERESIZE_SW;
      origimgpos = currimgpos;
      imagemovepos = imageresizepos = pos;
      lastsize = wxSize(currimg.GetWidth(), currimg.GetHeight());
    }
    break;
  }
  case NONE: 
    {
      // hand editing
      wxPoint polarcoord = getLEDForXYPoint(pos.x, pos.y);
      int lednum = polarcoord.x;
      int rownum = polarcoord.y;
      
      //wxLogDebug("%d %d", lednum, rownum);
      if ((lednum < 0) || (lednum > num_leds) || 
	  (rownum < 0) || (rownum > ROWS_PER_WHEEL))
	return;
      
      if (model[rownum][lednum] == false) {
	model[rownum][lednum] = true;
	drawstate = DRAWING;
      } else {
	model[rownum][lednum] = false;
	drawstate = ERASING;
      }
      Refresh();
      break;
    }
  }
}


void WheelPanel::OnMotion(wxMouseEvent &event) {


  switch (drawstate) {
  case NONE: {
#if defined(W32_NATIVE)
    wxPoint polarcoord = getLEDForXYPoint(event.GetPosition().x, event.GetPosition().y);
	if ((polarcoord.x >= 0) && (polarcoord.x < num_leds)) {
      wxSetCursor(wxCursor(wxCURSOR_PENCIL));
    } else {
      wxSetCursor(*wxSTANDARD_CURSOR);
    }
#endif
    return;
  }
  case IMAGEFOLLOW: {
    // not dragging
    wxPoint pos = event.GetPosition();
    // in the square?
    if (! origimg) 
      return;

    if ((pos.x >= currimgpos.x+10) && (pos.x <= currimgpos.x+currimg.GetWidth()-10) &&
	(pos.y >= currimgpos.y+10) && (pos.y <= currimgpos.y+currimg.GetHeight()-10)) {
      // change the cursor to hand
      this->SetCursor(wxCursor(wxCURSOR_HAND));
    } else if  ( ((pos.x >= currimgpos.x) && (pos.x < currimgpos.x+10) &&
		 (pos.y >= currimgpos.y) && (pos.y < currimgpos.y+10)) ||
		((pos.x > currimgpos.x+currimg.GetWidth()-10) && 
		 (pos.x < currimgpos.x+currimg.GetWidth()) &&
		 (pos.y > currimgpos.y+currimg.GetHeight()-10) && 
		 (pos.y < currimgpos.y+currimg.GetHeight())) ){
      // north west or south east
      this->SetCursor(wxCursor(wxCURSOR_SIZENWSE));
    } else if ( ((pos.x >= currimgpos.x+currimg.GetWidth()-10) && 
		 (pos.x < currimgpos.x+currimg.GetWidth()) &&
		 (pos.y >= currimgpos.y) && (pos.y < currimgpos.y+10)) ||
		((pos.x > currimgpos.x) && (pos.x < currimgpos.x+10) &&
		 (pos.y > currimgpos.y+currimg.GetHeight()-10) && 
		 (pos.y < currimgpos.y+currimg.GetHeight())) )
      {
      // North east or South West
	this->SetCursor(wxCursor(wxCURSOR_SIZENESW));
    } else {
      this->SetCursor(*wxSTANDARD_CURSOR);
    }
    break;
  }
  case IMAGEMOVE: {
    // set the current image position to follow
    //wxLogDebug(" to (%d, %d)", event.GetPosition().x, event.GetPosition().y);
    wxPoint testpos =  origimgpos + event.GetPosition() - imagemovepos;

    // dont let them move it off the screen!
    if ((testpos.x < 0) && (testpos.x+currimg.GetWidth() < TABSIZE))
      break;
    if ((testpos.x > GetSize().GetWidth()-TABSIZE) && (testpos.x+currimg.GetWidth() > GetSize().GetWidth()))
      break;
    if ((testpos.y < 0) && (testpos.y+currimg.GetHeight() < TABSIZE))
      break;
    if ((testpos.y > GetSize().GetHeight()-TABSIZE) && (testpos.y+currimg.GetHeight() > GetSize().GetHeight()))
      break;

    // ok
    currimgpos = origimgpos + event.GetPosition() - imagemovepos;
    //wxLogDebug("@(%d, %d)",currimgpos.x, currimgpos.y);
    Refresh();
    break;
  }
  case IMAGERESIZE_SE: {
    wxSize newsize = lastsize + wxSize(event.GetPosition().x, event.GetPosition().y) -
      wxSize(imageresizepos.x, imageresizepos.y);

    // is shift held down?
    if (shiftdown) {
      if (newsize.x > newsize.y) 
	newsize.x = newsize.y;
      else
	newsize.y = newsize.x;
    }

    //wxLogDebug("New size (%d, %d)", newsize.GetWidth(), newsize.GetHeight());

    if ((currimgpos.x < 0) && (currimgpos.x+newsize.GetWidth() < TABSIZE))
      break;
    if ((currimgpos.y < 0) && (currimgpos.y+newsize.GetHeight() < TABSIZE ))
      break;

    if ((newsize.GetWidth() <= 0) || (newsize.GetHeight() <= 0))
      break;
    currimg = origimg->Scale(newsize.GetWidth(), newsize.GetHeight());
    Refresh();
    break;
  }
  case IMAGERESIZE_NE: {
    wxSize newsize = lastsize + wxSize(event.GetPosition().x, -event.GetPosition().y) -
      wxSize(imageresizepos.x, -imageresizepos.y);

    int curry = origimgpos.y + event.GetPosition().y - imagemovepos.y;

    // is shift held down?
    if (shiftdown) {
      if (newsize.x > newsize.y) {
	newsize.x = newsize.y;
      }
      else {
	curry = origimgpos.y + event.GetPosition().y + (newsize.y-newsize.x) - imagemovepos.y;
	newsize.y = newsize.x;
      }
    }
    //    wxLogDebug("New size (%d, %d)", newsize.GetWidth(), newsize.GetHeight());

    // dont let them move it off screen
    if ((curry+newsize.GetHeight() > this->GetSize().GetHeight()) &&
	(curry+TABSIZE > this->GetSize().GetHeight()))
      break;
    if ((origimgpos.x < 0) && (origimgpos.x+newsize.GetWidth()-TABSIZE < 0))
      break;
    // dont allow negative sizes
    if ((newsize.GetWidth() <= 0) || (newsize.GetHeight() <= 0))
      break;
    // OK!
    currimg = origimg->Scale(newsize.GetWidth(), newsize.GetHeight());
    currimgpos.y = curry;
    Refresh();
    break;
  }
  case IMAGERESIZE_NW: {
    wxSize newsize = lastsize - wxSize(event.GetPosition().x, event.GetPosition().y) +
      wxSize(imageresizepos.x, imageresizepos.y);

    int curry = origimgpos.y + event.GetPosition().y - imagemovepos.y;
    int currx = origimgpos.x + event.GetPosition().x - imagemovepos.x;

    // is shift held down?
    if (shiftdown) {
      if (newsize.x > newsize.y) {
	currx = origimgpos.x + event.GetPosition().x + (newsize.x - newsize.y) - imagemovepos.x;
	newsize.x = newsize.y;
      } else {
	curry = origimgpos.y + event.GetPosition().y + (newsize.y-newsize.x) - imagemovepos.y;
	newsize.y = newsize.x;
      }
    }
    //wxLogDebug("New size (%d, %d)", newsize.GetWidth(), newsize.GetHeight());

    if ((curry+newsize.GetHeight() > this->GetSize().GetHeight()) &&
	(curry+TABSIZE > this->GetSize().GetHeight()))
      break;
    if ((currx+newsize.GetWidth() > this->GetSize().GetWidth()) &&
	(currx+TABSIZE > this->GetSize().GetWidth()))
      break;


    if ((newsize.GetWidth() <= 0) || (newsize.GetHeight() <= 0))
      break;
    currimg = origimg->Scale(newsize.GetWidth(), newsize.GetHeight());
    currimgpos.x = currx; currimgpos.y = curry;
    Refresh();
    break;
  }
  case IMAGERESIZE_SW: {
    wxSize newsize = lastsize + wxSize(-event.GetPosition().x, event.GetPosition().y) -
      wxSize(-imageresizepos.x, imageresizepos.y);

    int currx = origimgpos.x + event.GetPosition().x - imagemovepos.x; 
 
    // is shift held down?
    if (shiftdown) {
      if (newsize.x > newsize.y) {
	currx = origimgpos.x + event.GetPosition().x + (newsize.x - newsize.y) - imagemovepos.x; 
	newsize.x = newsize.y;
      }
      else
	newsize.y = newsize.x;
    }
    // wxLogDebug("New size (%d, %d)", newsize.GetWidth(), newsize.GetHeight());

    // dont let them resize beyond the window
    
    if ((currx+newsize.GetWidth() > this->GetSize().GetWidth()) &&
	(currx+TABSIZE > this->GetSize().GetWidth()))
      break;
    if ((origimgpos.y < 0) && (origimgpos.y+newsize.GetHeight()-TABSIZE < 0))
      break;
    
    // dont allow negative size
    if ((newsize.GetWidth() <= 0) || (newsize.GetHeight() <= 0))
      break;
    currimg = origimg->Scale(newsize.GetWidth(), newsize.GetHeight());
    currimgpos.x = currx;
    Refresh();
    break;
  }

  case DRAWING:
  case ERASING: {
#if defined(W32_NATIVE)
      wxSetCursor(wxCursor(wxCURSOR_PENCIL));
#endif
    wxPoint polar  = getLEDForXYPoint(event.GetPosition().x, event.GetPosition().y);
    
    int rownum = polar.y;
    int lednum = polar.x;

    if ((lednum < 0) || (lednum >= num_leds) || (rownum < 0) || (rownum >= ROWS_PER_WHEEL))
      return;
    
    if ((drawstate == DRAWING) && !model[rownum][lednum]) {
      model[rownum][lednum] = true;
      Refresh();
    } else if ((drawstate == ERASING) && model[rownum][lednum]) {
      model[rownum][lednum] = false;
      Refresh();
    }
    break;
  }
  }
}

void WheelPanel::OnEraseBackGround(wxEraseEvent& event) {};



wxPoint WheelPanel::getXYPointForLED(int led, int row) {
  wxSize size = GetClientSize();
  int width = size.x; int height = size.y;
  
  float dDiameter = (size.x < size.y) ? size.x : size.y;
  dDiameter /= (num_leds + hubsize);

  float radius = (led + .5 + hubsize)*dDiameter/2;
  float startangle = ((row+.5) * 360.0)/ROWS_PER_WHEEL;
  float angle = startangle * PI/180.0;
    
  return wxPoint(  int(sin(angle)*radius+(width/2)),int((height/2)-cos(angle)*radius));
}


wxPoint WheelPanel::getLEDForXYPoint(int x, int y) {
  wxSize size = GetClientSize();
  int lednum, rownum;
  int width = size.x; int height = size.y;

  float dDiameter = (size.x < size.y) ? size.x : size.y;
  dDiameter /= (num_leds + hubsize);

  // find led num by finding distance from point to middle
  lednum = int(sqrt(pow((width/2 - x), 2) + pow((height/2 - y), 2) ) / (dDiameter / 2) - hubsize);

  //find row by calculating angle
  x -= width/2;
  y -= height/2;

  float radius = sqrt(pow(x, 2) + pow(y, 2) );
  float angle = asin(x/radius) * 180/PI;

  angle = ((int)angle + 360) % 360;
  if ((x > 0) && (y > 0))
    angle = 180 - angle;
  if  ((x < 0) && (y > 0))
    angle = 360 - angle + 180;

  rownum = int(angle / 360 * ROWS_PER_WHEEL);

  return wxPoint(lednum, rownum);
}

wxString WheelPanel::GetSavedFilename(void) {
  return wxString(savedname);
}

void WheelPanel::SetSavedFilename(wxString name) {
  savedname = wxString(name);
}

BEGIN_EVENT_TABLE (WheelPanel, wxPanel) 
  EVT_PAINT(WheelPanel::OnPaint)
  EVT_ERASE_BACKGROUND(WheelPanel::OnEraseBackGround)
  EVT_LEFT_DOWN(WheelPanel::OnLeftDown)
  EVT_LEFT_UP(WheelPanel::OnLeftUp)
  EVT_MOTION(WheelPanel::OnMotion)

  EVT_KEY_UP(WheelPanel::OnChar)
  EVT_KEY_DOWN(WheelPanel::OnChar)

END_EVENT_TABLE()
