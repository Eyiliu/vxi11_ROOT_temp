{
  VXI11_CLINK *clink;
  tek_open(&clink, "192.168.227.27");
  tek_scope_init(clink);
  int npoints_want = 1000;
  int actual_npoints;
  actual_npoints = tek_scope_set_record_length(clink, npoints_want);
  cout<< "actual_npoints = " <<actual_npoints<<endl;
  int clear_sweeps = 1;
  int timeout = 10000;
  int buf_size;
  buf_size = tek_scope_set_for_capture(clink, clear_sweeps, timeout);
  // tek_scope_set_averages(clink, no_averages);

  cout<<"buf_size = "<<buf_size<<endl;
  
  char *buf;
  buf = new char[buf_size *4];

  int bytes_returned;
  bytes_returned = tek_scope_get_data(clink, "ch1, ch2", clear_sweeps,  buf, buf_size *4, timeout);

  cout<< "bytes_returend = "<< bytes_returned<<endl;
  
  int *ibuf = new int[actual_npoints];
  int *ibuf2 = new int[actual_npoints];

  int *nbuf = new int[actual_npoints];
  for(int i = 0; i<actual_npoints; i ++){
    ibuf[i] = buf[i*2] * 256 + buf[i*2+1];
    ibuf2[i] = buf[actual_npoints*2+i*2] * 256 + buf[actual_npoints*2+i*2+1];
    nbuf[i] = i;
    // if(buf[i] != 0){
    //   cout<< i << " "<< (int)buf[i]<<endl;
    // }
    
    
  }
  TGraph tg(actual_npoints, nbuf, ibuf );
  TGraph tg2(actual_npoints, nbuf, ibuf2 );
  TCanvas c1;
  c1.Divide(1,2);
  c1.cd(1);
  tg.Draw();
  c1.cd(2);
  tg2.Draw();



}

  // fwrite(buf, sizeof(char), bytes_returned, f_wf);
  
