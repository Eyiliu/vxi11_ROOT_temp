{
  vxi11_mso5000 *sco;
  VXI11_CLINK *clink;
  string ip("192.168.227.27"); //scope ip

  int clear_sweeps = 1;
  int timeout = 30000;
  int npoints_want = 1000;
  int actual_npoints;
  int ndig;
  int buf_size_ch;
  int buf_size;
  char *buf;

  sco = new vxi11_mso5000(ip.c_str());
  clink = sco->tek_scope_get_clink();  
  
  actual_npoints = sco->tek_scope_set_record_length(npoints_want);
  int *xbuf = new int[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    xbuf[i] = i;
  }



  buf_size_ch = sco->tek_scope_set_for_capture( clear_sweeps, timeout);
  buf_size = buf_size_ch * 4 + 100;
  buf = new char[buf_size];

  stringstream ss;
  ss<<buf_size_ch;
  ndig = ss.str().size();
  
  bytes_returned = sco->mytek_scope_get_data("ch1,ch2, ch3,ch4", 
					     clear_sweeps, buf, buf_size, timeout);
  
  double mvolpp[4];
  long pyoff[4];
  for(int i=0; i<4; i++){
    mvolpp[i] = sco->mytek_scope_get_vol_point(i+1) *1000;
    pyoff[i] = sco->mytek_scope_get_yoffset_point(i+1);
  }

  double samplerate = sco->tek_scope_get_sample_rate();
  double *xbufd = new double[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    xbufd[i] = i/(samplerate/1.e9);
  }



  vector<int> vbufch[4];
  for(int i = 0; i<4; i++){
    char *buf8ch = (char *)(buf+2+ndig+ i*(actual_npoints+2+ndig));
    vbufch[i].resize(actual_npoints);
    for(int j = 0; j<actual_npoints; j ++){
      vbufch[i][j] = buf8ch[j];
    }
  }

  vector<double> vmvch[4];
  for(int i=0; i<4; i++){ 
    int len = vbufch[i].size();
    vmvch[i].resize(len);
    for(int j = 0; j< len; j ++){
      vmvch[i][j] = (vbufch[i][j] - pyoff[i])*mvolpp[i];
    }
  }



  TCanvas c1;
  c1.Divide(1,4);
  for(int i=0; i<4; i++){
    c1.cd(i+1);
    TGraph *tg = new TGraph(actual_npoints, xbuf, &(vbufch[i][0]));
    tg->Draw();
  }


  TCanvas c2;
  c2.Divide(1,4);
  for(int i=0; i<4; i++){
    c2.cd(i+1);
    TGraph *tg = new TGraph(actual_npoints, xbufd, &(vmvch[i][0]));
    tg->Draw();
  }


}
