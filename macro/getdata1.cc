{
  vxi11_mso5000 *sco;
  VXI11_CLINK *clink;
  string ip("192.168.227.27");

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
  buf_size_ch = sco->tek_scope_set_for_capture( clear_sweeps, timeout);
  buf_size = buf_size_ch * 4 + 100;
  buf = new char[buf_size];

  stringstream ss;
  ss<<buf_size_ch;
  ndig = ss.str().size();
  
  bytes_returned = sco->mytek_scope_get_data("ch1,ch2, ch3,ch4", 
					     clear_sweeps, buf, buf_size, timeout);
  
  for(int i=0; i<10; i++){
    cout<<" "<<(int)buf[i]<<" ";
  }

  cout<<endl;

  for(int i=1000; i<1030; i++){
    cout<<" "<<(int)buf[i]<<" ";
  }

  cout<<endl;

  int *xbuf = new int[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    xbuf[i] = i;
  }


  // TCanvas c1;
  // c1.Divide(1,3);
  // c1.cd(1);
  // char *buf8c1 = (char *)(buf+2+ndig);
  // int *bufc1 = new int[actual_npoints];
  // for(int i = 0; i<actual_npoints; i ++){
  //   bufc1[i] = buf8c1[i];
  // }
  // TGraph tg1(actual_npoints, xbuf, bufc1);
  // tg1.Draw();


  // c1.cd(2);
  // char *buf8c2 = (char *)(buf+2+ndig+actual_npoints+2+ndig);
  // int *bufc2 = new int[actual_npoints];
  // for(int i = 0; i<actual_npoints; i ++){
  //   bufc2[i] = buf8c2[i];
  // }
  // TGraph tg2(actual_npoints, xbuf, bufc2);
  // tg2.Draw();


  // c1.cd(3);
  // char *buf8c3 = (char *)(buf+2+ndig+actual_npoints+2+ndig+actual_npoints+2+ndig);
  // int *bufc3 = new int[actual_npoints];
  // for(int i = 0; i<actual_npoints; i ++){
  //   bufc3[i] = buf8c3[i];
  // }
  // TGraph tg3(actual_npoints, xbuf, bufc3);
  // tg3.Draw();



  TCanvas c1;
  c1.Divide(1,2);
  c1.cd(1);
  int16_t *buf16c1 = (int16_t *)(buf+2+ndig);
  int *bufc1 = new int[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    bufc1[i] = buf16c1[i];
  }
  TGraph tg1(actual_npoints, xbuf, bufc1);
  tg1.Draw();


  c1.cd(2);
  int16_t *buf16c2 = (int16_t *)(buf+2+ndig+actual_npoints*2+2+ndig);
  int *bufc2 = new int[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    bufc2[i] = buf16c2[i];
  }
  TGraph tg2(actual_npoints, xbuf, bufc2);
  tg2.Draw();

}
