#ifndef _TEK_USER_H_
#define _TEK_USER_H_


#include<string>

#include "../vxi/vxi11_user.h"


class vxi11_mso5000{


public:
  vxi11_mso5000(const std::string ip_str);
  
  VXI11_CLINK* tek_scope_get_clink(){return clink;};
  int tek_scope_init();
  long tek_scope_set_for_capture(int clear_sweeps,unsigned long timeout);
  long tek_scope_calculate_no_of_bytes(unsigned long timeout);
  long tek_scope_get_data(char *source, int clear_sweeps,
			  char *buf, size_t len, unsigned long timeout);
  void tek_scope_set_for_auto();
  int tek_scope_set_averages(int no_averages);
  int tek_scope_get_averages();
  long tek_scope_set_record_length(long record_length);
  long tek_scope_get_no_points();
  double tek_scope_get_sample_rate();

  int tek_scope_select_channel(int ch, int on);
  
  long mytek_scope_get_data(std::string source, int clear_sweeps,
			  char *buf, size_t len, unsigned long timeout);

  // double mytek_scope_get_point_xinterval();
  double mytek_scope_get_vol_point(int ch);
  long mytek_scope_get_yoffset_point(int ch);

private:
  ~vxi11_mso5000();

  VXI11_CLINK *clink;
  std::string ip;

};


#endif
