#define MAX_BANKS 32

class SpokePOV_App: public wxApp
{
  virtual bool OnInit();
  // virtual void ExitMainLoop();
  virtual int OnExit();
  //virtual int OnRun();


 private:
  bool _exit;
  wxFrame *frame;
};

class  WheelPanel;
class SpokePOVComm;

class SpokePOVFrame : wxFrame {
 public:
  SpokePOVFrame(const wxString &title);

  bool LoadConfiguration(void);
  bool SaveConfiguration(void);
  bool SetConfigValue(const wxString key, const wxString value);
  wxString* GetConfigValue(const wxString key);

  void Save(int, wxString);

  void OnCloseWindow(wxCloseEvent& event);
  void OnExit(wxCommandEvent& evt);
  void OnAbout(wxCommandEvent& evt);
  void OnSave(wxCommandEvent &evt);
  void OnSaveAs(wxCommandEvent &evt);
  void OnOpen(wxCommandEvent &evt);
  void OnTestPort(wxCommandEvent &evt);
  void OnImport(wxCommandEvent &evt);

  void OnMirror(wxCommandEvent &evt);
  void OnHubSize(wxCommandEvent &evt);
  void OnCommDelay(wxCommandEvent &evt);

  void OnWheelSizeBMX(wxCommandEvent &evt);
  void OnWheelSizeRoad(wxCommandEvent &evt);

  void OnPortUSB(wxCommandEvent &evt);
  void OnPortSerial(wxCommandEvent &evt);
  void OnPortParallel(wxCommandEvent &evt);
  void OnParallelPortChange(wxCommandEvent &evt);
  void OnSerialPortChange(wxCommandEvent &evt);

  void OnButtonDone(wxCommandEvent &evt);
  void OnButtonWrite(wxCommandEvent &evt);
  void OnButtonRead(wxCommandEvent &evt);
  void OnButtonVerify(wxCommandEvent &evt);

  void OnChar(wxKeyEvent &event);
  void OnKey(wxKeyEvent &event);

  void OnConnect(wxCommandEvent &evt);

  void OnRotation(wxCommandEvent &evt);
  void OnAnimation(wxCommandEvent &evt);
  int Connect(void);

 DECLARE_EVENT_TABLE()
 private:
  wxConfig *cfgFile;

  SpokePOVComm *comm;

  wxMenuBar *menubar; 
  wxMenu *fileMenu, *configMenu, *porttypeMenu, *portMenuU, *portMenuS, *portMenuP, *wheelsizeMenu;
  WheelPanel *wheelpanel[MAX_BANKS];
  wxButton *connect, *rotation, *animation;
  wxNotebook *notebook;
  wxChoice *mirror;

  wxString commlink, commport_serial, commport_parallel;
  int num_leds, banks;
  int hub_size;
  int comm_delay;
};

