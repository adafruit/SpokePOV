#include <wx/panel.h>
#include <wx/image.h>
#include <wx/choice.h>



class WheelPanel : public wxPanel {
 public:
  WheelPanel(wxWindow *parent, wxPanel *ctl, int num_leds=30, int hub_size=5);
  void OnPaint(wxPaintEvent& event);
  void Draw(wxDC *dc);
  void OnEraseBackGround(wxEraseEvent& event);
  void OnChar(wxKeyEvent &event);
  void OnLeftDown(wxMouseEvent &event);
  void OnLeftUp(wxMouseEvent &event);
  void OnMotion(wxMouseEvent &event);
  wxPoint getLEDForXYPoint(int x, int y);
  wxPoint getXYPointForLED(int led, int row);

  wxString GetSavedFilename(void);
  void SetSavedFilename(wxString);
  bool SaveModel(void);
  bool LoadModel(wxString name);
  bool SetModel(unsigned char buff[]);
  bool GetModel(unsigned char buff[]);
  bool ImportBMP(wxString name);

  void setupRWVcontrols(void);
  void setupImageControl(void);
  void SetHubsize(int hs);

  int num_leds;
  int hubsize;
  
  wxBoxSizer *controlsizer;
  
DECLARE_EVENT_TABLE()
 private:
  bool shiftdown;
  int drawstate;
  wxImage currimg;
  wxImage *origimg;
  wxPoint origimgpos, currimgpos; // the offset for the background image
  wxPoint imagemovepos, imageresizepos; // where we grabbed the image to move it!
  wxSize lastsize;

  bool **model;

  wxString savedname;

  wxPanel *controlpanel;
  wxButton *Read, *Write, *Verify, *Done;
  wxStaticText *Donetext;
};
