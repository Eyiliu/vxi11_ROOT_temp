#ifndef _AGILENT_USER_H_
#define _AGILENT_USER_H_


#include <string>

#include "../vxi/vxi11_user.h"


class vxi11_dso7054{

    public:
        vxi11_dso7054(const std::string ip_str);

        VXI11_CLINK* agilent_scope_get_clink(){return clink;};

        int agilent_scope_init();

        int agilent_scope_extra(char channel, double voltage_range);

        int agilent_scope_get_setup(char *buf, size_t buf_len);

        int agilent_scope_send_setup(char *buf, size_t buf_len);

        long agilent_scope_calculate_no_of_bytes(char chan,
                unsigned long timeout);

        int agilent_scope_set_for_capture(double s_rate, long npoints, 
                unsigned long timeout);

        void agilent_scope_set_for_auto();

        long agilent_scope_get_n_points();

        long agilent_scope_get_data(char chan, char *buf, size_t buf_len,
                unsigned long timeout);

        int agilent_scope_display_channel(char chan, int on_or_off);

        long agilent_scope_get_data(char chan, int digitise, char *buf, 
                size_t buf_len, unsigned long timeout);

        int agilent_scope_report_status(unsigned long timeout);

        void agilent_scope_channel_str(char chan, char *source);

        long agilent_scope_get_screen_data(char chan, char *buf,
                size_t buf_len, unsigned long timeout,
                double s_rate, long npoints);

        int agilent_scope_get_preamble(char *buf, size_t buf_len);

        int agilent_scope_set_averages(int no_averages);

        long agilent_scope_get_averages();

        double agilent_scope_get_sample_rate();

        long agilent_scope_write_wfi_file(char *wfiname, char chan, char *captured_by, int no_of_traces, unsigned long timeout);

        long agilent_scope_write_wfi_file(char *wfiname, long no_of_bytes, char *captured_by, int no_of_traces, unsigned long timeout);

        void agilent_scope_get_information(char chan, double & vpp, double & vmax, double & vmin);

    private:
        ~vxi11_dso7054();

        VXI11_CLINK *clink;
        std::string ip;

};

#endif
