#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include "imagemessagebox.h"

BEGIN_EVENT_TABLE(ImageMessageBox, wxDialog )
  EVT_BUTTON( wxID_YES, ImageMessageBox::OnYes )
  EVT_BUTTON( wxID_NO, ImageMessageBox::OnNo)
END_EVENT_TABLE()

void ImageMessageBox::OnYes(wxCommandEvent& event) {
  if ( Validate() && TransferDataFromWindow() )
    {
      if ( IsModal() )
	EndModal(wxYES); // If modal
      else
	{
	  SetReturnCode(wxYES);
	  this->Show(false); // If modeless
	}
    }
}

void ImageMessageBox::OnNo(wxCommandEvent& event) {
  if ( Validate() && TransferDataFromWindow() )
    {
      if ( IsModal() )
	EndModal(wxNO); // If modal
      else
	{
	  SetReturnCode(wxNO);
	  this->Show(false); // If modeless
	}
    }
}

ImageMessageBox::ImageMessageBox(wxWindow *parent, wxString title, 
				 wxString text, wxBitmap img, int style)  {
  if (! wxDialog::Create(parent, wxID_ANY, title))
    return;

  image = new wxStaticBitmap(this, wxID_ANY, img);

  wxBoxSizer *dlgsizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(dlgsizer);
  dlgsizer->Add(image, 0, wxALL|wxALIGN_CENTER, 5);
  while (text.Len() != 0) {
    int index = text.Find('\n');
    if (index != -1) {
      wxStaticText *message = new wxStaticText(this, wxID_ANY, text.Mid(0, index+1));
      dlgsizer->Add(message, 0, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxRIGHT, 10);
      text = text.Mid(index+1);
    } else {
      wxStaticText *message = new wxStaticText(this, wxID_ANY, text);
      dlgsizer->Add(message, 0, wxEXPAND|wxALIGN_LEFT|wxLEFT|wxRIGHT, 10);
      text = "";
    }
  }

  wxSizer *buttonbox = CreateButtonSizer(style);
  dlgsizer->Add(buttonbox, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10);
  
  wxSizer *sizer = GetSizer();
  sizer->SetSizeHints(this);
  /*  
wxLogDebug("message %d", message->GetSize().GetHeight());
  sizer->SetMinSize(wxSize(sizer->GetMinSize().GetWidth(), 
			   img.GetHeight() + message->GetSize().GetHeight()+10)); 
  wxLogDebug("message2 %d", message->GetSize().GetHeight());
  
  sizer->SetSizeHints(this);
  wxLogDebug("message3 %d", message->GetSize().GetHeight());


  wxSize s ;
  s = message->GetBestSize();
  wxLogDebug("%d, %d", s.GetWidth(), s.GetHeight());
  s = message->GetAdjustedBestSize();
  wxLogDebug("%d, %d", s.GetWidth(), s.GetHeight());
  */
  Centre();
  return;
}

