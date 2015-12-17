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

  vector<double> vqmvns[4];
  
  vector<int> vbufch[4];
  vector<double> vmvch[4];
  double qmvns[4];
  std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>goto loop"<<endl;  
  for(int n=0; n<200; n++){
    std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>loop 1"<<endl;
    std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>loop 2"<<endl;


    std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>loop 3"<<endl;

    std::cout<<"preparing: goto mytek_scope_get_data()"<<std::endl;
    sco->mytek_scope_get_data("ch1,ch2, ch3,ch4", 
			      clear_sweeps, buf, buf_size, timeout);
    std::cout<<"finished: goto mytek_scope_get_data()"<<std::endl;
    std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>loop 4"<<endl;


    for(int i = 0;i<4; i++){
      char *buf8ch = (char *)(buf+2+ndig+ i*(actual_npoints+2+ndig));
      vbufch[i].resize(actual_npoints);
      for(int j = 0; j<actual_npoints; j ++){
	vbufch[i][j] = buf8ch[j];
      }
    }
    
    for(int i=0; i<4; i++){ 
      int len = vbufch[i].size();
      qmvns[i] = 0;
      vmvch[i].resize(len);
      for(int j = 0; j< len; j ++){
	int mv = (vbufch[i][j] - pyoff[i])*mvolpp[i];
	vmvch[i][j] = mv ;
	if(mv < 5){
	  qmvns[i] -= mv/(samplerate/1.e9); 
	}
      }
    }

    for(int i=0; i<4; i++){
      vqmvns[i].push_back(qmvns[i]);
    }
    cout<<"qmvns = "<<qmvns[0] <<"  "<<qmvns[1] <<"  "<<qmvns[2] <<"  "<<qmvns[3] <<endl;


    std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>loop end??"<<endl;
  }


  TCanvas c3;
  c3.Divide(2,2);
  TH1F *th[4];
  for(int i=0; i<4; i++){
    c3.cd(i+1);
    th[i] = new TH1F("h1", "h1 title", 50, 0, 1000);
    for(int j=0; j<vqmvns[i].size(); j++){
      th[i]->Fill(vqmvns[i][j]);
    }
    th[i]->Draw();
  }


  
  
  // TCanvas c1;
  // c1.Divide(1,4);
  // for(int i=0; i<4; i++){
  //   c1.cd(i+1);
  //   TGraph *tg = new TGraph(actual_npoints, xbuf, &(vbufch[i][0]));
  //   tg->Draw();
  // }


  // TCanvas c2;
  // c2.Divide(1,4);
  // for(int i=0; i<4; i++){
  //   c2.cd(i+1);
  //   TGraph *tg = new TGraph(actual_npoints, xbufd, &(vmvch[i][0]));
  //   tg->Draw();
  // }


}
