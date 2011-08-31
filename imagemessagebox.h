class ImageMessageBox : public wxDialog {

  DECLARE_EVENT_TABLE()
 public: 
  ImageMessageBox(wxWindow *parent,  wxString title,  wxString text, wxBitmap img, int style=(wxYES|wxNO|wxCANCEL));
  void OnYes(wxCommandEvent& event);
  void OnNo(wxCommandEvent& event);

 private:
  wxStaticBitmap *image;
};
