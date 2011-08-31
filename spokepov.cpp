#include <sys/types.h>
#include <dirent.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/log.h>
#include <wx/debug.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/image.h>
#include <wx/filedlg.h>
#include <wx/stattext.h>
#include <wx/config.h>
#include <wx/statbmp.h>
#include <wx/progdlg.h>
#include <wx/numdlg.h>
#include <wx/choice.h>
#include <wx/dir.h>

#include "spokepov.h"
#include "imagemessagebox.h"
#include "wheelpanel.h"
#include "communication.h"
#include "globals.h"

#include <wx/mstream.h>
#include "resc/parrconnect.png.h"
#include "resc/dataprobe.png.h"
#include "resc/clkprobe.png.h"
#include "resc/resetprobe.png.h"
#include "resc/serialprobe3.png.h"
#include "resc/serialprobe4.png.h"
#include "resc/serialprobe7.png.h"
#include "resc/parrprobe2.png.h"
#include "resc/parrprobe4.png.h"
#include "resc/parrprobe5.png.h"

#define wxGetBitmapFromMemory(name) _wxGetBitmapFromMemory(name ## _png, sizeof(name ## _png))

inline wxBitmap _wxGetBitmapFromMemory(const unsigned char *data, int length) {
  wxMemoryInputStream is(data, length);
  return wxBitmap(wxImage(is, wxBITMAP_TYPE_ANY, -1), -1);
}


IMPLEMENT_APP(SpokePOV_App)

bool SpokePOV_App::OnInit()
{
  wxLogDebug("hi");

  this->_exit = false;

  new SpokePOVFrame("wxSpokePOV");
  wxImage::AddHandler(new wxPNGHandler);

  return true;
} 

int SpokePOV_App::OnExit() {
  
  return 0;
}

bool SpokePOVFrame::LoadConfiguration(void) {
  cfgFile = new wxConfig("SpokePOV");
  wxConfig::Set(cfgFile);
  return true;
}

bool SpokePOVFrame::SaveConfiguration(void) {
  cfgFile->Flush();
  return true;
}


wxString* SpokePOVFrame::GetConfigValue(const wxString key) {
  wxString *ret = new wxString();
  if (cfgFile->Read(key, ret))
    return ret;
  else
    return NULL;
}

bool SpokePOVFrame::SetConfigValue(const wxString key, const wxString value) {
  return cfgFile->Write(key, value);
}

SpokePOVFrame::SpokePOVFrame(const wxString &title)
  :  wxFrame((wxFrame *)NULL, wxID_ANY, title,
	     wxPoint(150, 150),
	     wxSize(750, 550),
	     (wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE))
{
  wxString *key;
  LoadConfiguration();
  key = GetConfigValue("num_leds");
  if (key == NULL)
    num_leds = 30;
  else {
    long foo;
    if (key->ToLong(&foo))
      num_leds = foo;
    else
      num_leds = 30;
  }
  wxLogDebug("Config: %d LEDs", num_leds);
  SetConfigValue("num_leds", wxString::Format("%d",num_leds));
  
  key = GetConfigValue("hub_size");
  if (key == NULL)
    hub_size = 5;
  else {
    long foo;
    if (key->ToLong(&foo))
      hub_size = foo;
    else
      hub_size = 5;
  }
  wxLogDebug("Config: %d\" hub", hub_size);
  SetConfigValue("hub_size", wxString::Format("%d",hub_size));

  
  key = GetConfigValue("comm_delay");
  if (key == NULL)
    comm_delay = 500;
  else {
    long foo;
    if (key->ToLong(&foo))
      comm_delay = foo;
    else
      comm_delay = 500;
  }
  wxLogDebug("Delay: %d\" us", comm_delay);
  SetConfigValue("comm_delay", wxString::Format("%d",comm_delay));

  key = GetConfigValue("commlink");
#if defined (W32_NATIVE)
  if (key == NULL)  {
    commlink = wxString("parallel");
  } else if ((key->IsSameAs("parallel")) || (key->IsSameAs("serial")) || (key->IsSameAs("usb"))) {
    commlink = wxString(*key);
    wxLogDebug("Config: %s", commlink.c_str());
  } else {
    commlink = wxString("parallel");
    wxLogDebug("Config: assuming parallel");
  }
#else
  if (key == NULL)  {
    commlink = wxString("serial");
  } else if (key->IsSameAs("serial") || key->IsSameAs("usb")) {
    commlink = wxString(*key);
    wxLogDebug("Config: %s", commlink.c_str());
  } else {
    commlink = wxString("serial");
    wxLogDebug("Config: assuming serial");
  }
#endif

  SetConfigValue("commlink", commlink);

  key = GetConfigValue("commport_serial");
  if (key == NULL) {
#if defined (W32_NATIVE)
    commport_serial = "COM1";
#else
	commport_serial = wxFindFirstFile("/dev/cu.*");
#endif
    wxLogDebug("Config: undef. port, will choose first available");
  } else {
    commport_serial = wxString(*key);
    wxLogDebug("Config serial port"+commport_serial);
  }
  SetConfigValue("commport_serial", commport_serial);

#if defined (W32_NATIVE)
  key = GetConfigValue("commport_parallel");
  if (key == NULL) {
    commport_parallel = "lpt1";
    wxLogDebug("Config: undef. port, will choose first available");
  } else {
    commport_parallel = wxString(*key);
    wxLogDebug("Config parr port"+commport_parallel);
  }
  SetConfigValue("commport_parallel", commport_parallel);
#endif
  
  SaveConfiguration();

  banks = 0;

  menubar = new wxMenuBar();
  SetMenuBar(menubar);
  fileMenu = new wxMenu();
  
  fileMenu->Append(wxID_ABOUT, "About SpokePOV",
		   "About SpokePOV");

  fileMenu->AppendSeparator();
  fileMenu->Append(ID_FILE_OPEN, "Open...", "Open a SpokePOV image file");
  fileMenu->Append(ID_FILE_IMPORT, "Import BMP...\tCTRL-I",  "Import a BMP file");
  fileMenu->Append(ID_FILE_SAVE, "Save",  "Save SpokePOV image to a file");
  fileMenu->Append(ID_FILE_SAVEAS, "Save As...",  "Save the SpokePOV image to a new file");
  fileMenu->Enable(ID_FILE_OPEN, false);
  fileMenu->Enable(ID_FILE_IMPORT, false);
  fileMenu->Enable(ID_FILE_SAVE, false);
  fileMenu->Enable(ID_FILE_SAVEAS, false);

  fileMenu->AppendSeparator();
  fileMenu->Append(wxID_EXIT, "Quit\tCTRL-Q", 
		   "Exit the Program");
  menubar->Append(fileMenu, "File");
  
  
  configMenu = new wxMenu();
  porttypeMenu = new wxMenu();

  porttypeMenu->Append(ID_PORTTYPE_USB, "USB",
		      "USB", wxITEM_RADIO);
  portMenuU = new wxMenu();

#if defined (W32_NATIVE)
  portMenuP = new wxMenu();
  portMenuP->Append(ID_PORT_LPT1, "lpt1", "LPT1", wxITEM_RADIO);
  portMenuP->Append(ID_PORT_LPT2, "lpt2", "LPT2", wxITEM_RADIO);
  portMenuP->Append(ID_PORT_LPT3, "lpt3", "LPT3", wxITEM_RADIO);
  portMenuP->Check(ID_PORT_LPT1, true);
  porttypeMenu->Append(ID_PORTTYPE_PARALLEL, "Parallel",
		      "Parallel Port", wxITEM_RADIO);
#endif

  portMenuS = new wxMenu();
#if defined (W32_NATIVE)
  for (int com=1; com<10; com++) {
    wxString name = wxString::Format("COM%d",com);
    portMenuS->Append(ID_PORT_SERIAL+com-1, name, name, wxITEM_RADIO);
  }
#else
	wxLogDebug("Finding serial ports");
	wxArrayString files;
	/*
	// good fucking god, wxdir doesnt support devfs...
	int num = wxDir::GetAllFiles("/dev", &files);
	wxLogDebug("%d files", num);
	*/
	// i hate it but no choice
	DIR *dir = opendir("/dev");
	struct dirent *file;
	wxLogDebug("reading /dev");
	int i = 0;
	while ((file = readdir(dir)) != NULL) {
		wxString filename(file->d_name);
		if (filename.StartsWith("cu.")) {
			filename = "/dev/"+filename;
			portMenuS->Append(ID_PORT_SERIAL+i, filename, filename, wxITEM_RADIO);
			i++;
			wxLogDebug(wxString("found port ")+filename);
		}
	}
	closedir(dir);
#endif

  porttypeMenu->Append(ID_PORTTYPE_SERIAL, "Serial",
		      "Serial Port", wxITEM_RADIO);
  configMenu->Append(ID_PORTTYPEMENU, "Port Type", porttypeMenu);


  if (commlink == "parallel") {
    porttypeMenu->Check(ID_PORTTYPE_PARALLEL, true);
    configMenu->Append(ID_PORTMENU, "Port", portMenuP);
  } else if (commlink == "serial") {
    porttypeMenu->Check(ID_PORTTYPE_SERIAL, true);
    configMenu->Append(ID_PORTMENU, "Port", portMenuS);
  } else {
    porttypeMenu->Check(ID_PORTTYPE_USB, true);
    configMenu->Append(ID_PORTMENU, "Port", portMenuU);
    configMenu->Enable(ID_PORTMENU, false);
  }

  comm = 0;

  wheelsizeMenu = new wxMenu();
  wheelsizeMenu->Append(ID_WHEELSIZE_BMX, "BMX",
		       "BMX (16\") wheel", wxITEM_RADIO);
  wheelsizeMenu->Append(ID_WHEELSIZE_ROAD, "MTB/Road",
		       "MTB/Road size wheel", wxITEM_RADIO);
  if (num_leds == 30)
    wheelsizeMenu->Check(ID_WHEELSIZE_ROAD, true);
  else if (num_leds == 22)
    wheelsizeMenu->Check(ID_WHEELSIZE_BMX, true);

  configMenu->Append(ID_WHEELSIZEMENU, "Wheel Size", wheelsizeMenu);
  configMenu->Append(ID_HUBSIZE, "Hub Size...");
  configMenu->AppendSeparator();
  configMenu->Append(ID_TESTPORT, "Test Port...");
  configMenu->Append(ID_COMMDELAY, "Comm Delay...");


  menubar->Append(configMenu, "Config");

  // put in the divider panel
  wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
  topsizer->SetSizeHints(this);
  SetSizer(topsizer);

  wxPanel *toppanel = new wxPanel(this);
  topsizer->Add(toppanel, 0, wxEXPAND);
  wxBoxSizer *toppanelsizer = new wxBoxSizer(wxHORIZONTAL);
  toppanel->SetSizer(toppanelsizer);
  wxStaticText *label = new wxStaticText(toppanel, wxID_ANY, "SpokePOV Soft");
  toppanelsizer->Add(label, 0, wxEXPAND|wxALL, 10);
  wxFont f = label->GetFont();
  f.SetPointSize(14);
  label->SetFont(f);
  connect = new wxButton(toppanel, ID_BUTTON_CONNECT, "Connect");
  toppanelsizer->Add(connect, 0, wxALL|wxALIGN_RIGHT, 10);

  rotation = new wxButton(toppanel, BUTTON_ID_SETROT, "Rotation Offset...");
  toppanelsizer->Add(rotation, 0, wxALL|wxALIGN_RIGHT, 10);

  animation = new wxButton(toppanel, BUTTON_ID_SETANIM, "Animation Timing...");
  toppanelsizer->Add(animation, 0, wxTOP|wxBOTTOM|wxALIGN_RIGHT, 10);

  wxArrayString choices;
  choices.Add("Don't mirror");
  choices.Add("Mirror");
  mirror = new wxChoice(toppanel, CHOICE_ID_MIRROR, wxDefaultPosition, wxDefaultSize, choices);
  toppanelsizer->Add(mirror, 0, wxALL|wxALIGN_RIGHT, 10);

  notebook = new wxNotebook(this, wxID_ANY, wxPoint(50, 50), wxSize(500, 500));
  topsizer->Add(notebook, 1, wxEXPAND);

  for (int x=0; x<4; x++)
    wheelpanel[x] = NULL;

  Show(true);
  SetSize(600,600);
  animation->Show(false);
  rotation->Show(false);
  mirror->Show(false);
}


void SpokePOVFrame::OnCloseWindow(wxCloseEvent& event) {
  wxLogDebug("Window Closed");
  Destroy();
}


void SpokePOVFrame::OnExit(wxCommandEvent& evt) {
  wxLogDebug("Exit!");
  Close(true);
}

void SpokePOVFrame::OnAbout(wxCommandEvent& evt) {
  wxMessageBox("SpokePOV software v1.4\nUtility software for controlling and configuring SpokePOV kits.\nPlease visit http://www.ladyada.net/make/spokepov for more \ninformation about SpokePOV hardware and software!\n\n(c) 2006 Adafruit Industries\nDistributed under Creative Commons\nSupported by EYEBEAM", "About SpokePOV", wxOK|wxICON_INFORMATION, this);
}


void SpokePOVFrame::OnSave(wxCommandEvent& evt) {
  Save(notebook->GetSelection(), wheelpanel[notebook->GetSelection()]->GetSavedFilename());
}

void SpokePOVFrame::OnSaveAs(wxCommandEvent& evt) {
  Save(notebook->GetSelection(), "");
}

void SpokePOVFrame::Save(int currbank, wxString filename) {
  if (filename == "") {
    wxFileDialog *filedialog = new wxFileDialog(this,
				     "Save SpokePov image...", // title
				     "", "",          // no defaults
				     "Data file (*.dat)|*.dat",  // Datafile only 
				     wxSAVE);        // save a file

    if (filedialog->ShowModal() == wxID_CANCEL) {
      wxLogDebug("Canceled");
      return; // canceled, bail
    }

    // ok they saved
    wxString filename = filedialog->GetPath();
    notebook->SetPageText(notebook->GetSelection(), 
			  wxString::Format("Bank %d: ",currbank+1) + filedialog->GetFilename());
    wheelpanel[currbank]->SetSavedFilename(filename);
  }
  wxLogDebug(filename);
  wheelpanel[currbank]->SaveModel();
}    


void SpokePOVFrame::OnOpen(wxCommandEvent &evt) {
  wxString filename;
  wxFileDialog *filedialog = new wxFileDialog(this,
					      "Open SpokePOV image...", // title
					      "", "",          // no defaults
					      "Data file (*.dat)|*.dat",  // Datafile only 
					      wxOPEN);        // save a file
  
  if (filedialog->ShowModal() == wxID_CANCEL) {
    wxLogDebug("Canceled");
    return; // canceled, bail
  }

  int currbank = notebook->GetSelection();

  // ok they opened
  filename = filedialog->GetPath();
  wxLogDebug(filename);
  if (wheelpanel[currbank]->LoadModel(filename))
    notebook->SetPageText(notebook->GetSelection(), wxString::Format("Bank %d: ", currbank+1)+filedialog->GetFilename());
}    


void SpokePOVFrame::OnImport(wxCommandEvent &evt) {
  wxString filename;
  wxFileDialog *filedialog = new wxFileDialog(this,
					      "Open BMP image...",
					      "", "",
					      "BMP file (*.bmp)|*.bmp", 
					      wxOPEN);  
  if (filedialog->ShowModal() == wxID_CANCEL) {
    wxLogDebug("Canceled");
    return; // canceled, bail
  }

  int currbank = notebook->GetSelection();

  // find out current panel 
  // ok they opened
  filename = filedialog->GetPath();
  wxLogDebug(filename);
  wheelpanel[currbank]->ImportBMP(filename);
}


void SpokePOVFrame::OnRotation(wxCommandEvent &evt) {
  int i;
  unsigned char c;
  
  if (!comm->readbyte(EEPROM_ROTOFFSET, &c)) {
    wxMessageBox("Failed to read rotation offset!\nYou may want to try increasing the communications delay", "Failed", wxICON_HAND|wxOK);
    return;
  }
 i = ::wxGetNumberFromUser("Enter new rotation offset.\nThis will depend on where the magnet is placed \nin respect to 'up.' \nSee the website for some suggested values", "Rotation (0-255)", "Set Rotation Offset", c, 0, 255);
 if (i == -1)
   return;
 c = i;

 if (!comm->writebyte(EEPROM_ROTOFFSET, c)) {
    wxMessageBox("Failed to write rotation offset!", "Failed", wxICON_HAND|wxOK);
 }
}

void SpokePOVFrame::OnAnimation(wxCommandEvent &evt) {
  int i;
  unsigned char c;
  
  if (!comm->readbyte(EEPROM_ANIMTIME, &c)) {
    wxMessageBox("Failed to read animation timing!\nYou may want to try increasing the communications delay", "Failed", wxICON_HAND|wxOK);
    return;
  }
  i = ::wxGetNumberFromUser("Enter new animation frame timing. \nThat is, how many wheel rotations should \neach 'frame' take before the next one is displayed?", "# frames (0-255)", "Set animation timing", c, 0, 255);
 if (i == -1)
   return;
 c = i;

 if (!comm->writebyte(EEPROM_ANIMTIME, c)) {
    wxMessageBox("Failed to write animcation timing!", "Failed", wxICON_HAND|wxOK);
 }
}

void SpokePOVFrame::OnConnect(wxCommandEvent &evt) {
  if (connect->GetLabel() == "Disconnect") {
    connect->SetLabel("Connect");
    fileMenu->Enable(ID_FILE_OPEN, false);
    fileMenu->Enable(ID_FILE_IMPORT, false);
    fileMenu->Enable(ID_FILE_SAVE, false);
    fileMenu->Enable(ID_FILE_SAVEAS, false); 
    configMenu->Enable(ID_COMMDELAY, true); 

    mirror->Show(false);
    rotation->Show(false);
    animation->Show(false);
    GetSizer()->Layout();
   
    notebook->DeleteAllPages();
    for (int x=0; x<4; x++)
      wheelpanel[x] = NULL;
    banks = 0;

    delete this->comm;
    this->comm = 0;
    return;
  }

  if (Connect() != 0) {
    wxMessageBox("Failed to communicate with SpokePOV!\nYou may want to try increasing the communications delay", "Failed", wxICON_HAND|wxOK, this); 
    return;
  }
  connect->SetLabel("Disconnect");
  fileMenu->Enable(ID_FILE_OPEN, true);
  fileMenu->Enable(ID_FILE_IMPORT, true);
  fileMenu->Enable(ID_FILE_SAVE, true);
  fileMenu->Enable(ID_FILE_SAVEAS, true);
  configMenu->Enable(ID_COMMDELAY, false); 

  rotation->Show(true);
  animation->Show(true);
  mirror->Show(true);
  GetSizer()->Layout();
}

int SpokePOVFrame::Connect(void) {
  unsigned char ret;
  wxString commport;

  wxLogDebug("Connecting via "+commlink);
  if (commlink == "serial")
    commport = commport_serial;
  else if (commlink == "usb")
    commport = "USB";
  else
    commport = commport_parallel;
   
  wxProgressDialog progdlg("Connecting to spokePOV", "Attempting to find SpokePOV on "+commlink+" port "+commport+"..."); 
 // open up connection
  if (this->comm && this->comm->IsOK()) {
    delete this->comm;
    this->comm = 0;
  }

  this->comm = CommFactory(commlink, commport);
  
  if (! this->comm->IsOK()) {
    wxMessageBox("Failed to open the port", "Failed", wxICON_HAND|wxOK, this);
    return -2;
  }
 
  unsigned char byte[6];
  /*
#if defined (WIN32_NATIVE)
  for (int delay=1; delay <=5; delay++) {
#else
  for (int delay=4; delay <=6; delay++) {
#endif
  */
  comm->reset(); // reset
  wxMilliSleep(1000);
  comm->delay = comm_delay;
  /*if (comm->readbyte(0, &byte1))
    break;
    wxLogDebug("trying longer delay %d", delay+1);
    }*/
  
  // check up to 32KB 
  for (int i = 0; i<6; i++) {
    if (!comm->readbyte((1<<i)*1024, &byte[i]))
      return -1;
  }

  wxLogDebug("%d %d %d %d %d %d", byte[0], byte[1], byte[2], byte[3], byte[4], byte[5] );

  banks = 0;

  // is this a 1x memory?
  for (int j=0; j<6; j++) {
    wxLogDebug("test for %d banks", 1<<j);
    for (int i=j; i<6; i++) {
      if (byte[i] != byte[0])
	break;
    }
    // possibly!
    comm->writebyte(0, ~byte[0]); // invert one byte
    if (!comm->readbyte(1024*(1<<j), &ret)) {
      return -1;
    }
    if (ret == ((unsigned char)(~byte[0]))) {
      banks = 1<<j; // yes!
      break;
    } else {
      wxLogDebug("0x%02x 0x%02x", ret, (unsigned char)~byte[0]);
    }
    if (!comm->writebyte(0, byte[0])) {
      return -1;
    }
  }

  wxLogDebug("found %d banks of memory", banks);

  wxSize foo = GetSize();

  // for # of banks...
  for (int i=0; i < banks; i++) {
    wxPanel *bankpanel = new wxPanel(notebook);
    wxBoxSizer *banksizer = new wxBoxSizer(wxVERTICAL);
    banksizer->SetSizeHints(this);
    bankpanel->SetSizer(banksizer);
    notebook->AddPage(bankpanel, wxString::Format("Bank %d: Untitled", i+1));

    wxPanel *controlpanel = new wxPanel(bankpanel);
    wheelpanel[i] = new WheelPanel(bankpanel, controlpanel, num_leds, hub_size);  
    banksizer->Add(wheelpanel[i], 1, wxEXPAND);
    banksizer->Add(controlpanel, 0, wxEXPAND);
    
    wheelpanel[i]->controlsizer->SetSizeHints(this);

  }
  SetSize(foo);

  unsigned char mir = 0;
  comm->readbyte(EEPROM_MIRROR, &mir);
  if (mir == 0) {
    mirror->SetSelection(0);
  } else if (mir == 1) {
    mirror->SetSelection(1);
  }

  return 0;
}


void SpokePOVFrame::OnTestPort(wxCommandEvent &evt) {
  int ret;
  wxString commport;
 

  wxLogDebug("Testing");
  if (commlink == "serial")
    commport = commport_serial;
  if (commlink == "usb")
    commport = "USB";
  else
    commport = commport_parallel;
  
  if (wxMessageBox("Do you want to test the SpokePOV dongle connected to "+commlink+" port "+commport+"?", "Test", wxOK|wxCANCEL|wxICON_QUESTION, this) == wxCANCEL)
    return;

  // open up connection
  if (this->comm && this->comm->IsOK()) {
    delete this->comm;
    this->comm = 0;
  }

  this->comm = CommFactory(commlink, commport);

  if (! this->comm->IsOK()) {
    wxMessageBox("Failed to open the port", "Failed", wxICON_HAND|wxOK, this);
    return;
  }
  ImageMessageBox dlg9(this, 
		      wxString("Connect dongle"),
		      "OK, connect up the dongle (only) to the "+commlink+" port "+commport+".",
		      wxGetBitmapFromMemory(parrconnect), wxCANCEL|wxOK);
  if (dlg9.ShowModal() == wxCANCEL)
    return;
  
  wxString parrerror = wxString::Format("Measure the voltage on pin #%%d of the parallel port\nIf it is %%s then there is some problem with the dongle between that pin and the header.\nIf it is not, then there is some problem with the parallel port.\nSome things to try:\n   * Verify you are plugged into the correct port ("+commport+").\n   * Try a different cable, or plugging in directly (if possible)\n   * Finally, check the BIOS to make sure that the address is set to 0x%x!\n",  comm->getfd());
  wxString sererror = wxString::Format("Measure the voltage on pin #%%d of the serial port\nIf it is %%s then there is some problem with the dongle between that pin and the header.\nIf it is not, then there is some problem with the serial port.\nSome things to try:\n   * Verify you are plugged into the correct port ("+commport+").\n   * Try a different cable or adaptor, or plugging in directly (if possible)\n");

  comm->setpin(MOSI, true);
  
  ImageMessageBox dlg(this, 
		      wxString("Testing DATAOUT pin"),
		      wxString("Use a multimeter to read the voltage on pin #1 of the box header, is it > 3.0 Volts?"),
		      wxGetBitmapFromMemory(dataprobe));
  if ((ret = dlg.ShowModal()) == wxID_CANCEL)
    return;
  if (ret == wxNO) {
    if (commlink == "parallel") {
      ImageMessageBox dlg(this,	wxString("Parallel port problems"),
			  wxString::Format(parrerror, 2, _T("> 3.0 V")),
			  wxGetBitmapFromMemory(parrprobe2), wxOK);
      dlg.ShowModal();
    }
    else if (commlink == "serial") {
      ImageMessageBox dlg(this, 
			  wxString("Test serial port"),
			  wxString::Format(sererror, 3, _T("> 3.0 V")),
			   wxGetBitmapFromMemory(serialprobe3), wxOK);
      dlg.ShowModal();
    }
    return;
  }
    
  comm->setpin(MOSI, false);
 
  ImageMessageBox dlg2(this, wxString("Testing DATAOUT pin"),
      wxString("Use a multimeter to read the voltage on pin #1 of the box header, is it < 0.5 Volts?"),
     wxGetBitmapFromMemory(dataprobe));
  if ((ret = dlg2.ShowModal()) == wxID_CANCEL)
    return;

  if (ret == wxNO) {
    if (commlink == "parallel"){
      ImageMessageBox dlg(this, wxString("Parallel port problems"),
			  wxString::Format(parrerror, 2, _T("< 0.5 V")),
			   wxGetBitmapFromMemory(parrprobe2), wxOK);
      dlg.ShowModal();
    }
    else if (commlink == "serial") {
      ImageMessageBox dlg(this, 
			  wxString("Test serial port"),
			  wxString::Format(sererror, 3, _T("< 0 V")),
			  wxGetBitmapFromMemory(serialprobe3), wxOK);
      dlg.ShowModal();
    }
    
    return;
  }

   comm->setpin(RESET, true);

   ImageMessageBox dlg5(this, wxString("Testing RESET pin"),
			"Use a multimeter to read the voltage on pin #5 of the box header, is it > 3.0 Volts?",
			wxGetBitmapFromMemory(resetprobe));
   if ((ret = dlg5.ShowModal()) == wxID_CANCEL)
     return;
   if (ret == wxNO) {
     if (commlink == "parallel") {
       ImageMessageBox dlg(this, wxString("Parallel port problems"),
			   wxString::Format(parrerror, 4, _T("> 3.0 V")),
			   wxGetBitmapFromMemory(parrprobe4), wxOK);
      dlg.ShowModal();
    }  else if (commlink == "serial") {
      ImageMessageBox dlg(this, 
			  wxString("Test serial port"),
			  wxString::Format(sererror, 7, _T("> 3.0 V")),
			   wxGetBitmapFromMemory(serialprobe7), wxOK);
      dlg.ShowModal();
     }
     return;
   }

   comm->setpin(RESET, false);

   ImageMessageBox dlg6(this, wxString("Testing RESET pin"),
			"Use a multimeter to read the voltage on pin #5 of the box header, is it < 0.5 Volts?",
			 wxGetBitmapFromMemory(resetprobe));
   if ((ret = dlg6.ShowModal()) == wxID_CANCEL)
     return;
   if (ret == wxNO) {
     if (commlink == "parallel") {
       ImageMessageBox dlg(this, wxString("Parallel port problems"),
			   wxString::Format(parrerror, 4, _T("< 0.5 V")),
			   wxGetBitmapFromMemory(parrprobe4), wxOK);
      dlg.ShowModal();
    } else if (commlink == "serial") {
       ImageMessageBox dlg(this, 
			   wxString("Test serial port"),
			   wxString::Format(sererror, 7, _T("< 0 V")),
			   wxGetBitmapFromMemory(serialprobe7), wxOK);
       dlg.ShowModal();
     }
     return;
   }

  comm->setpin(CLK, true);

  ImageMessageBox dlg3(this, wxString("Testing CLOCK pin"),
      wxString("Use a multimeter to read the voltage on pin #7 of the box header, is it > 3.0 Volts?"),
		       wxGetBitmapFromMemory(clkprobe));
  if ((ret = dlg3.ShowModal()) == wxID_CANCEL)
      return;

   if (ret == wxNO) {
     if (commlink == "parallel")
      {
	ImageMessageBox dlg(this, wxString("Parallel port problems"),
			    wxString::Format(parrerror, 5, _T("> 3.0 V")),
			    wxGetBitmapFromMemory(parrprobe5), wxOK);
	dlg.ShowModal();
      } else if (commlink == "serial") {
       ImageMessageBox dlg(this, 
			   wxString("Test serial port"),
			   wxString::Format(sererror, 4, _T("> 3.0 V")),
			   wxGetBitmapFromMemory(serialprobe4), wxOK);
       dlg.ShowModal();
     }
     return;
   }

   comm->setpin(CLK, false);
   
   ImageMessageBox dlg4(this, wxString("Testing CLOCK pin"),
       wxString("Use a multimeter to read the voltage on pin #7 of the box header, is it < 0.5 Volts?"),
			wxGetBitmapFromMemory(clkprobe));
   if ((ret = dlg4.ShowModal()) == wxID_CANCEL)
     return;
   if (ret == wxNO) {
     if (commlink == "parallel") {
       ImageMessageBox dlg(this,  wxString("Parallel port problems"),
			   wxString::Format(parrerror, 5, _T("< 0.5 V")),
			   wxGetBitmapFromMemory(parrprobe5), wxOK);
       dlg.ShowModal();
     } else if (commlink == "serial") {
       ImageMessageBox dlg(this, 
			   wxString("Test serial port"),
			   wxString::Format(sererror, 4, _T("< 0 V")),
			   wxGetBitmapFromMemory(serialprobe4), wxOK);
       dlg.ShowModal();
     }
     return;
   }

   /*
     ret=wxMessageBox("Jumper the input low", "Testing MISO pin, when you're done, press OK", wxOK|wxCANCEL, this);
     if (ret == wxCANCEL)
     return;
     wxLogDebug("read %d", comm->getpin(MISO));
     
     ret=wxMessageBox("Jumper the input high", "Testing MISO pin, when you're done, press OK", wxOK|wxCANCEL, this);
     if (ret == wxCANCEL)
     return;
     wxLogDebug("read %d", comm->getpin(MISO));
   */
   
   wxMessageBox("SpokePOV communication is ready to go!","Passed",  wxOK);
}

void SpokePOVFrame::OnButtonDone(wxCommandEvent &evt) {
  wxLogDebug("Done");
  // get the current page...

  int currbank = notebook->GetSelection();
  wheelpanel[currbank]->setupRWVcontrols();
}

void SpokePOVFrame::OnWheelSizeBMX(wxCommandEvent &evt) {
  SetConfigValue("num_leds", wxString::Format("%d",22));
  SaveConfiguration();
  for (int i = 0; i<banks; i++) {
    wheelpanel[i]->num_leds = 22;
    wheelpanel[i]->Refresh();
  }
}
void SpokePOVFrame::OnWheelSizeRoad(wxCommandEvent &evt) {
  SetConfigValue("num_leds", wxString::Format("%d",30));
  SaveConfiguration();
  for (int i = 0; i<banks; i++) {
    wheelpanel[i]->num_leds = 30;
    wheelpanel[i]->Refresh();
  }
}


void SpokePOVFrame::OnPortUSB(wxCommandEvent &evt) {
  SetConfigValue("commlink", "usb");
  commlink = "usb";
  SaveConfiguration();
  configMenu->Enable(ID_PORTMENU, false);
}

void SpokePOVFrame::OnPortSerial(wxCommandEvent &evt) {
  SetConfigValue("commlink", "serial");
  commlink = "serial";
  SaveConfiguration();
  configMenu->Enable(ID_PORTMENU, true);
  for (unsigned int i=0; i<configMenu->GetMenuItemCount(); i++) {
    if ((configMenu->FindItemByPosition(i)->GetSubMenu() == portMenuU) ||
	(configMenu->FindItemByPosition(i)->GetSubMenu() == portMenuP))
      {
	configMenu->Insert(i, ID_PORTMENU, "Port", portMenuS);
	configMenu->Remove(configMenu->FindItemByPosition(i+1));
      }
  }
 
  bool found = false;
  for (unsigned int i=0; i<portMenuS->GetMenuItemCount(); i++) {
    if (portMenuS->FindItemByPosition(i)->GetText() == commport_serial) {
      portMenuS->FindItemByPosition(i)->Check(true);
      found = true;
    }
  }

  if (!found && (portMenuS->GetMenuItemCount() > 0)) {
    portMenuS->FindItemByPosition(0)->Check(true);
    commport_serial = portMenuS->FindItemByPosition(0)->GetText();
    SetConfigValue("commport_serial", commport_serial);
    SaveConfiguration();
  }
}

void SpokePOVFrame::OnPortParallel(wxCommandEvent &evt) {
  SetConfigValue("commlink", "parallel");
  commlink = "parallel";
  SaveConfiguration();
  configMenu->Enable(ID_PORTMENU, true);
  // remove old port menu
  for (unsigned int i=0; i<configMenu->GetMenuItemCount(); i++) {
    if ((configMenu->FindItemByPosition(i)->GetSubMenu() == portMenuS) ||
	(configMenu->FindItemByPosition(i)->GetSubMenu() == portMenuU)) {
      configMenu->Insert(i, ID_PORTMENU, "Port", portMenuP);
      configMenu->Remove(configMenu->FindItemByPosition(i+1));
    }
  }

  
  bool found = false;
  for (unsigned int i=0; i<portMenuP->GetMenuItemCount(); i++) {
    if (portMenuP->FindItemByPosition(i)->GetText() == commport_parallel) {
      portMenuP->FindItemByPosition(i)->Check(true);
      found = true;
    }
  }
  if (!found && (portMenuP->GetMenuItemCount() > 0)) {
    portMenuP->FindItemByPosition(0)->Check(true);
    commport_parallel = portMenuP->FindItemByPosition(0)->GetText();
    SetConfigValue("commport_parallel", commport_parallel);
    SaveConfiguration();
  }
}


void SpokePOVFrame::OnParallelPortChange(wxCommandEvent &evt) {
  for (unsigned int i=0; i<portMenuP->GetMenuItemCount(); i++)
    if (portMenuP->FindItemByPosition(i)->IsChecked())
      commport_parallel = portMenuP->FindItemByPosition(i)->GetText();
  SetConfigValue("commport_parallel", commport_parallel);
  SaveConfiguration();
}



void SpokePOVFrame::OnSerialPortChange(wxCommandEvent &evt) {
  for (unsigned int i=0; i<portMenuS->GetMenuItemCount(); i++)
    if (portMenuS->FindItemByPosition(i)->IsChecked())
      commport_serial = portMenuS->FindItemByPosition(i)->GetText();
  SetConfigValue("commport_serial", commport_serial);
  wxLogDebug("new serial port: "+commport_serial);
  SaveConfiguration();
}

void SpokePOVFrame::OnButtonWrite(wxCommandEvent &evt) {
  unsigned char buff[1024];
  wxLogDebug("Write");
 // which bank?
  int currbank = notebook->GetSelection();

  wxProgressDialog progdlg("Connecting to spokePOV", wxString::Format("Writing image to bank #%d: %d/1024", currbank+1, 0), 1024); 
  
  wheelpanel[currbank]->GetModel(buff);

  for (int i=0; i< 1024; i+=16) {
    if (! comm->write16byte(currbank*1024+i, buff+i)) {
      wxMessageBox("Failed to write to SpokePOV!\nYou may want to try increasing the communications delay", 
		   "Failed", wxICON_HAND|wxOK);
      return;
    }
    progdlg.Update(i, wxString::Format("Writing image to bank #%d: %d/1024", currbank+1, i));
  }
  
}
void SpokePOVFrame::OnButtonRead(wxCommandEvent &evt) {
  unsigned char buff[1024];
  wxLogDebug("Read");

  // which bank?
  int currbank = notebook->GetSelection();
  wxProgressDialog progdlg("Talking to SpokePOV", wxString::Format("Reading image from bank #%d: %d/1024", currbank+1, 0), 1024); 

  for (int i=0; i< 1024; i+=16) {
    if (!     comm->read16byte(currbank*1024+i, buff+i)) {
      wxMessageBox("Failed to read from SpokePOV!\nYou may want to try increasing the communications delay", 
		   "Failed", wxICON_HAND|wxOK);
      return;
    }

    progdlg.Update(i, wxString::Format("Reading image from bank #%d: %d/1024", currbank+1, i));
  }
  wheelpanel[currbank]->SetModel(buff);
}

void SpokePOVFrame::OnButtonVerify(wxCommandEvent &evt) {
  unsigned char buff[1024], minibuff[16];
  wxLogDebug("Verify");

  // which bank?
  int currbank = notebook->GetSelection();
  wheelpanel[currbank]->GetModel(buff); // read the model into the buffer

  wxProgressDialog progdlg("Talking to spokePOV", wxString::Format("Verifying image in bank #%d: %d/1024", currbank+1, 0), 1024); 


  bool diff = false;
  for (int i=0; i< 1024; i+=16) {
    if (! comm->read16byte(currbank*1024+i, minibuff)) {
      wxMessageBox("Failed to read from SpokePOV!\nYou may want to try increasing the communications delay", 
		   "Failed", wxICON_HAND|wxOK);
      return;
    }

    progdlg.Update(i, wxString::Format("Verifying image in bank #%d: %d/1024", currbank+1, i));
    for (int j=0; j<16; j++) { // ugh, memcmp???
      if (buff[i+j] != minibuff[j]) {
	wxLogDebug(wxString::Format("Failed at addr %d: %x v. %x", i+j, buff[i+j], minibuff[j])); 
	diff = true;
	break;
      }
    }
    if (diff)
      break;
  }
  if (diff)
    wxMessageBox("Verify failed!", "Failed", wxICON_HAND|wxOK, this);
  else
    wxMessageBox("Verify succeeded!", "Succeeded",wxICON_EXCLAMATION|wxOK, this);
}

 void SpokePOVFrame::OnMirror(wxCommandEvent &evt) {
  if (evt.GetInt() == 0) {
    // dont mirror
    if (!comm->writebyte(EEPROM_MIRROR, 0)) {
      wxMessageBox("Failed to write mirror!", "Failed", wxICON_HAND|wxOK);
    }
  } else if (evt.GetInt() == 1) {
        if (!comm->writebyte(EEPROM_MIRROR, 1)) {
      wxMessageBox("Failed to write mirror!", "Failed", wxICON_HAND|wxOK);
    }
  }
}

 void SpokePOVFrame::OnHubSize(wxCommandEvent &evt) {
   int i = ::wxGetNumberFromUser("Enter new hub diameter size in inches", "Hub diameter: ", "Set hub diameter", hub_size, 0, 20);
   if (i == -1)
     return;
   hub_size = i;

   for (int i = 0; i<banks; i++) {
     wheelpanel[i]->hubsize = hub_size;
     wheelpanel[i]->Refresh();
     
   }
   SetConfigValue("hub_size", wxString::Format("%d",hub_size));
 }


 void SpokePOVFrame::OnCommDelay(wxCommandEvent &evt) {
   int i = ::wxGetNumberFromUser("Enter new comm port delay.", "Delay (us): ", "Set comm delay", comm_delay, 0, 10000);
   if (i == -1)
     return;
   comm_delay = i;

   SetConfigValue("comm_delay", wxString::Format("%d",comm_delay));
 }

BEGIN_EVENT_TABLE (SpokePOVFrame, wxFrame) 
  EVT_CLOSE(SpokePOVFrame::OnCloseWindow)

  EVT_MENU(ID_PORTTYPE_USB, SpokePOVFrame::OnPortUSB)
  EVT_MENU(ID_PORTTYPE_SERIAL, SpokePOVFrame::OnPortSerial)
  EVT_MENU(ID_PORTTYPE_PARALLEL, SpokePOVFrame::OnPortParallel)

  EVT_MENU(ID_PORT_LPT1, SpokePOVFrame::OnParallelPortChange)
  EVT_MENU(ID_PORT_LPT2, SpokePOVFrame::OnParallelPortChange)
  EVT_MENU(ID_PORT_LPT3, SpokePOVFrame::OnParallelPortChange)
  EVT_MENU(ID_PORT_SERIAL, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+1, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+2, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+3, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+4, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+5, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+6, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+7, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+8, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+9, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+10, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+11, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+12, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+13, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+14, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+15, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+16, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+17, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+18, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+19, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+20, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+21, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+22, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+23, SpokePOVFrame::OnSerialPortChange)
  EVT_MENU(ID_PORT_SERIAL+24, SpokePOVFrame::OnSerialPortChange)

  EVT_MENU(ID_WHEELSIZE_BMX, SpokePOVFrame::OnWheelSizeBMX)
  EVT_MENU(ID_WHEELSIZE_ROAD, SpokePOVFrame::OnWheelSizeRoad)

  EVT_MENU(ID_HUBSIZE, SpokePOVFrame::OnHubSize)
  EVT_MENU(ID_COMMDELAY, SpokePOVFrame::OnCommDelay)

  EVT_MENU(wxID_ABOUT, SpokePOVFrame::OnAbout)
  EVT_MENU(ID_FILE_SAVE, SpokePOVFrame::OnSave)
  EVT_MENU(ID_FILE_SAVEAS, SpokePOVFrame::OnSaveAs)
  EVT_MENU(ID_FILE_OPEN, SpokePOVFrame::OnOpen)
  EVT_MENU(ID_TESTPORT, SpokePOVFrame::OnTestPort)
  EVT_MENU(ID_FILE_IMPORT, SpokePOVFrame::OnImport)
  EVT_MENU(wxID_EXIT, SpokePOVFrame::OnExit)

  EVT_BUTTON(BUTTON_ID_DONE, SpokePOVFrame::OnButtonDone)
  EVT_BUTTON(BUTTON_ID_READ, SpokePOVFrame::OnButtonRead)
  EVT_BUTTON(BUTTON_ID_WRITE, SpokePOVFrame::OnButtonWrite)
  EVT_BUTTON(BUTTON_ID_VERIFY, SpokePOVFrame::OnButtonVerify)
  EVT_CHOICE(CHOICE_ID_MIRROR, SpokePOVFrame::OnMirror)

  EVT_BUTTON(ID_BUTTON_CONNECT, SpokePOVFrame::OnConnect)
  EVT_BUTTON(BUTTON_ID_SETANIM, SpokePOVFrame::OnAnimation)
  EVT_BUTTON(BUTTON_ID_SETROT, SpokePOVFrame::OnRotation)

END_EVENT_TABLE()
