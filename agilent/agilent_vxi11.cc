#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "agilent_vxi11.h"




vxi11_dso7054::vxi11_dso7054(const std::string ip_str)
{
	ip = ip_str;
	vxi11_open_device(&clink, ip.c_str(), NULL);
}


vxi11_dso7054::~vxi11_dso7054()
{
	vxi11_close_device(clink, ip.c_str());
}

int vxi11_dso7054::agilent_scope_init()
{
        int ret;
        ret = vxi11_send_printf(clink, ":SYSTEM:HEADER 0");
        if (ret < 0) {
                printf("error in agilent init, could not send command '%s'\n",
                       ":SYSTEM:HEADER 0");
                return ret;
        }
        vxi11_send_printf(clink, "*RST");
        vxi11_send_printf(clink, ":AUTOSCALE");
        vxi11_send_printf(clink, ":ACQUIRE:MODE ETIM");
        vxi11_send_printf(clink, ":ACQUIRE:TYPE NORMAL");
        vxi11_send_printf(clink, ":ACQUIRE:COMPLETE 100");
        vxi11_send_printf(clink, ":WAVEFORM:BYTEORDER LSBFIRST");
        vxi11_send_printf(clink, ":WAVEFORM:FORMAT WORD");
        return 0;
}

int vxi11_dso7054::agilent_scope_extra(char channel, double voltage_range)
{
		vxi11_send_printf(clink, ":CHANnel%c:RANGe %f", channel, voltage_range); // voltage_range default unit V
		//vxi11_send_printf(clink, ":TIMEbase:RANGe %f", time_range); // time_range default unit ms
        return 0;
} 


/* See comments for above function, and comments within this function, for a
 * description of what it does.
 */
int vxi11_dso7054::agilent_scope_set_for_capture(double s_rate, long npoints, unsigned long timeout)
{
	long actual_npoints;	/* actual number of points returned */
	double time_range;
	double auto_srat;	/* sample rate whilst on auto setting */
	long auto_npoints;	/* no of points whilst on auto setting */
	double expected_s_rate;	/* based on s_rate passed to us, or npoints */
	double actual_s_rate;	/* what it ends up as */
	double xinc;		/* xincrement (only need for ETIM mode) */
	char etim_result[256];
	int cmp;
	int ret_val = 0;
	int not_enough_memory = 0;

	/* First we need to find out if we're in "ETIM" (equivalent time) mode,
	 * because things are done differently. You can't set the sample rate,
	 * and if you query it, you get a meaningless answer. You must work out
	 * what the effective sample rate is from the waveform xincrement. A
	 * pain in the arse, quite frankly. */

	vxi11_send_and_receive(clink, ":ACQ:MODE?", etim_result, 256,
			       VXI11_READ_TIMEOUT);

	/* Equivalent time (ETIM) mode: */
	if (strncmp("ETIM", etim_result, 4) == 0) {
		/* Find out the time range displayed on the screen */
		time_range = vxi11_obtain_double_value(clink, ":TIM:RANGE?");

		/* Find the xincrement, whilst we're still in auto (points) mode */
		auto_npoints = vxi11_obtain_long_value(clink, ":ACQ:POINTS?");

		/* Set the no of acquisition points to manual */
		vxi11_send_printf(clink, ":ACQ:POINTS:AUTO 0");

		if (npoints <= 0) {	// if we've not been passed a value for npoints
			npoints = auto_npoints;
		}
		/* Remember we want at LEAST the number of points specified.
		 * To some extent, the xinc value is determined by the
		 * number of points. So to get the best xinc value we ask
		 * for double what we actually want. */
		vxi11_send_printf(clink, ":ACQ:POINTS %ld", (2 * npoints) - 1);

		/* Unfortunately we have to do a :dig, to make sure our changes have
		 * been registered */
		vxi11_send_printf(clink, ":DIG");

		/* Find the xincrement is now */
		xinc = vxi11_obtain_double_value_timeout(clink, ":WAV:XINC?", timeout);

		/* Work out the number of points there _should_ be to cover the time range */
		actual_npoints = (long)((time_range / xinc) + 0.5);

		/* Set the number of points accordingly. Hopefully the
		 * xincrement won't have changed! */
		vxi11_send_printf(clink, ":ACQ:POINTS %ld", actual_npoints);

		/* This is a bit anal... we can work out very easily what the equivalent
		 * sampling rate is (1 / xinc); the scope seems to store this value
		 * somewhere, even though it doesn't use it. We may as well write it
		 * to the scope, in case some user program asks for it while in
		 * equivalent time mode. Should not be depended upon, though! */

		vxi11_send_printf(clink, ":ACQ:SRAT %G", (1 / xinc));
	}

	/* Real time (RTIM, NORM or PDET) mode: */
	else {
		/* First find out what the sample rate is set to.
		 * Each time you switch from auto to manual for either of these, the
		 * scope remembers the values from last time you set these manually.
		 * This is not very useful to us. We want to be able to set either the
		 * sample rate (and derive npoints from that and the timebase), or the
		 * minimum number of points (and derive the sample rate) or let
		 * the scope choose sensible values for both of these. We only want to
		 * capture the data for the time period displayed on the scope screen,
		 * which is equal to the time range. If you leave the scope to do
		 * everything auto, then it always acquires a bit more than what's on
		 * the screen.
		 */
		auto_srat = vxi11_obtain_double_value(clink, ":ACQ:SRAT?");

		/* Set the sample rate (SRAT) and no of acquisition points to manual */
		vxi11_send_printf(clink, ":ACQ:SRAT:AUTO 0;:ACQ:POINTS:AUTO 0");

		/* Find out the time range displayed on the screen */
		time_range = vxi11_obtain_double_value(clink, ":TIM:RANGE?");

		/* If we've not been passed a sample rate (ie s_rate <= 0) then... */
		if (s_rate <= 0) {
			/* ... if we've not been passed npoints, let scope set rate */
			if (npoints <= 0) {
				s_rate = auto_srat;
			}
			/* ... otherwise set the sample rate based on no of points. */
			else {
				s_rate = (double)npoints / time_range;
			}
		}
		/* We make a note here of what we're expecting the sample rate to be.
		 * If it has to change for any reason (dodgy value, or not enough
		 * memory) we will know about it.
		 */
		expected_s_rate = s_rate;

		/* Now we set the number of points to acquire. Of course, the scope
		 * may not have enough memory to acquire all the points, so we just
		 * sit in a loop, reducing the sample rate each time, until it's happy.
		 */
		do {
			/* Send scope our desired sample rate. */
			vxi11_send_printf(clink, ":ACQ:SRAT %G", s_rate);
			/* Scope will choose next highest allowed rate.
			 * Find out what this is */
			actual_s_rate =
			    vxi11_obtain_double_value(clink, ":ACQ:SRAT?");

			/* Calculate the number of points on display (and round up for rounding errors) */
			npoints = (long)((time_range * actual_s_rate) + 0.5);

			/* Set the number of points accordingly */
			/* Note this won't necessarily be the no of points you receive, eg if you have
			 * sin(x)/x interpolation turned on, you will probably get more. */
			vxi11_send_printf(clink, ":ACQ:POINTS %ld", npoints);

			/* We should do a check, see if there's enough memory */
			actual_npoints =
			    vxi11_obtain_long_value(clink, ":ACQ:POINTS?");

			if (actual_npoints < npoints) {
				not_enough_memory = 1;
				ret_val = -1;	/* We should report this fact to the calling function */
				s_rate =
				    s_rate * 0.75 *
				    (double)((double)actual_npoints /
					     (double)npoints);
			} else {
				not_enough_memory = 0;
			}
		} while (not_enough_memory == 1);
		/* Will possibly remove the explicit printf's here, maybe leave it up to the
		 * calling function to spot potential problems (the user may not care!) */
		if (actual_s_rate != expected_s_rate) {
			//printf("Warning: the sampling rate has been adjusted,\n");
			//printf("from %g to %g, because ", expected_s_rate, actual_s_rate);
			if (ret_val == -1) {
				//printf("there was not enough memory.\n");
			} else {
				//printf("because %g Sa/s is not a valid sample rate.\n",expected_s_rate);
				ret_val = -2;
			}
		}
	}
	return ret_val;
}

/* Return the scope to its auto condition */
void vxi11_dso7054::agilent_scope_set_for_auto()
{
	//vxi11_send_printf(clink, ":ACQ:SRAT:AUTO 1;:ACQ:POINTS:AUTO 1;:RUN");
	vxi11_send_printf(clink, ":RUN");
}

/* Get no of points (may not necessarily relate to no of bytes directly!) */
long vxi11_dso7054::agilent_scope_get_n_points()
{
	return vxi11_obtain_long_value(clink, ":ACQ:POINTS?");
}


/* Wrapper function; implicitely does the :dig command before grabbing the data
 * (safer but possibly slower, if you've already done a dig to get your wfi_file
 * info) */
long vxi11_dso7054::agilent_scope_get_data(char chan, char *buf,
		      size_t buf_len, unsigned long timeout)
{
	return vxi11_dso7054::agilent_scope_get_data(chan, 1, buf, buf_len, timeout);
}

long vxi11_dso7054::agilent_scope_get_data(char chan, int digitise, char *buf,
		      size_t buf_len, unsigned long timeout)
{
    char source[20];
	int ret;
	long bytes_returned;

    memset(source, 0, 20);
    vxi11_dso7054::agilent_scope_channel_str(chan, source);
	ret = vxi11_send_printf(clink, ":WAV:SOURCE %s", source);
	if (ret < 0) {
		printf
		    ("error, could not send :WAV:SOURCE %s cmd, quitting...\n",
		     source);
		return ret;
	}

	if (digitise != 0) {
		ret = vxi11_send_printf(clink, ":DIG");
	}

    do {
		ret = vxi11_send_printf(clink, ":WAV:DATA?");
		bytes_returned =
		    vxi11_receive_data_block(clink, buf, buf_len, timeout);
	} while (bytes_returned == -VXI11_NULL_READ_RESP);
	/* We have to check for this after a :DIG because it could take a very long time */
	return bytes_returned;
}

void vxi11_dso7054::agilent_scope_channel_str(char chan, char *source)
{
	switch (chan) {
	case '1':
		strcpy(source, "CHAN1");
		break;
	case '2':
		strcpy(source, "CHAN2");
		break;
	case '3':
		strcpy(source, "CHAN3");
		break;
	case '4':
		strcpy(source, "CHAN4");
		break;
	default:
		printf("error: unknown channel '%c', using channel 1\n", chan);
		strcpy(source, "CHAN1");
		break;
	}
}

/* Debugging function whilst developing the library. Might as well leave it here */
int vxi11_dso7054::agilent_scope_report_status(unsigned long timeout)
{
	char buf[256];
	double dval;
	long lval;

	memset(buf, 0, 256);
	if (vxi11_send_and_receive(clink, ":wav:source?", buf, 256, timeout) !=
	    0)
		return -1;
	printf("Source:        %s", buf);

	dval = vxi11_obtain_double_value_timeout(clink, ":tim:range?", timeout);
	printf("Time range:    %g (%g us)\n", dval, (dval * 1e6));

	dval = vxi11_obtain_double_value_timeout(clink, ":acq:srat?", timeout);
	lval = vxi11_obtain_long_value_timeout(clink, ":acq:srat:auto?", timeout);
	printf("Sample rate:   %g (%g GS/s), set to ", dval, (dval / 1e9));
	if (lval == 0)
		printf("MANUAL\n");
	else
		printf("AUTO\n");

	lval = vxi11_obtain_long_value_timeout(clink, ":acq:points?", timeout);
	printf("No of points:  %ld, set to ", lval);
	lval = vxi11_obtain_long_value_timeout(clink, ":acq:points:auto?", timeout);
	if (lval == 0)
		printf("MANUAL\n");
	else
		printf("AUTO\n");

	lval = vxi11_obtain_long_value_timeout(clink, ":acq:INT?", timeout);
	if (lval == 0)
		printf("(sin x)/x:     OFF\n");
	else
		printf("(sin x)/x:     ON\n");

	dval = vxi11_obtain_double_value_timeout(clink, ":wav:xinc?", timeout);
	printf("xinc:          %g (%g us, %g ps)\n", dval, (dval * 1e6),
	       (dval * 1e12));

	return 0;
}

/* Gets the system setup for the scope, dumps it in an array. Can be saved for later. */
int vxi11_dso7054::agilent_scope_get_setup(char *buf, size_t buf_len)
{
	int ret;
	long bytes_returned;

	ret = vxi11_send_printf(clink, ":SYSTEM:SETUP?");
	if (ret < 0) {
		printf("error, could not ask for system setup, quitting...\n");
		return ret;
	}
	bytes_returned =
	    vxi11_receive_data_block(clink, buf, buf_len, VXI11_READ_TIMEOUT);

	return (int)bytes_returned;
}

/* Sends a previously saved system setup back to the scope. */
int vxi11_dso7054::agilent_scope_send_setup(char *buf, size_t buf_len)
{
	int ret;
	long bytes_returned;

	ret = vxi11_send_data_block(clink, ":SYSTEM:SETUP ", buf, buf_len);
	if (ret < 0) {
		printf("error, could not send system setup, quitting...\n");
		return ret;
	}
	return 0;
}

/* Fairly important function. Calculates the number of bytes that digitised
 * data will take up, based on timebase settings, whether interpolation is
 * turned on etc. Can be used before you actually acquire any data, to find
 * out how big you need your data buffer to be. */
long vxi11_dso7054::agilent_scope_calculate_no_of_bytes(char chan,
				   unsigned long timeout)
{
	char source[20];
	double hinterval, time_range;
	double srat;
	long no_of_bytes;
	char etim_result[256];

	// First we need to digitize, to get the correct values for the
	// waveform data. This is a pain in the arse.
    vxi11_dso7054::agilent_scope_channel_str(chan, source);
	vxi11_send_printf(clink, ":WAV:SOURCE %s", source);
	vxi11_send_printf(clink, ":DIG");

	/* Now find the info we need to calculate the number of points */
	hinterval = vxi11_obtain_double_value_timeout(clink, ":WAV:XINC?", timeout);
	time_range = vxi11_obtain_double_value(clink, ":TIM:RANGE?");

	/* Are we in equivalent time (ETIM) mode? If so, the value of ACQ:SRAT will
	 * be meaningless, and there's a different formula */
	vxi11_send_and_receive(clink, ":ACQ:MODE?", etim_result, 256,
			       VXI11_READ_TIMEOUT);
	/* Equivalent time (ETIM) mode: */
	if (strncmp("ETIM", etim_result, 4) == 0) {
		no_of_bytes = (long)(2 * ((time_range / hinterval) + 0.5));
	} else {
		srat = vxi11_obtain_double_value(clink, ":ACQ:SRAT?");

		no_of_bytes =
		    (long)(2 * (((time_range - (1 / srat)) / hinterval) + 1) + 0.5);
		/* 2x because 2 bytes per data point
		 * +0.5 to round up when casting as a long
		 * -(1/srat) and +1 so that both raw data, and interpolated (sinx/x) data works */
	}
	return no_of_bytes;
}

/* Turns a channel on or off (pass "1" for on, "0" for off)*/
int vxi11_dso7054::agilent_scope_display_channel(char chan, int on_or_off)
{
	char source[20];

	memset(source, 0, 20);
    vxi11_dso7054::agilent_scope_channel_str(chan, source);
	return vxi11_send_printf(clink, ":%s:DISP %d", source, on_or_off);
}

/* The following function is more a demonstration of the sequence of capturing
 * data, than a useful function in itself (though it can be used as-is).
 * Agilent scopes, when either sample rate or no of acquisition points (or both)
 * are set to "auto", sets the time period for acquisition to be greater than
 * what is displayed on the screen. We find this way of working confusing, and
 * so we set the sample rate and no of acquisition points manually, such that
 * the acquisition period matches the time range (i.e. the period of time
 * displayed on the screen).
 * We may also want to control either the sample rate or the no of acquisition
 * points (we can't control both, otherwise that would change the time range!);
 * or, we may want the scope to select sensible values for us. So we have a
 * function agilent_set_for_capture() which sets everthing up for us.
 * Once this is done, we have a very basic (and hence fast) function that
 * actually digitises the data in binary form, agilent_get_data().
 * Finally, we like to return the scope back to automatic mode, whereby it
 * chooses sensible values for the sample rate and no of acquisition points,
 * and also restarts the acquisition (which gets frozen after digitisation).
 *
 * If you would like to grab the preamble information - see the function
 * agilent_get_preamble() - or write a wfi file - agilent_write_wfi_file() -
 * make sure that you do this BEFORE you return the scope to automatic mode.
 */
long vxi11_dso7054::agilent_scope_get_screen_data(char chan, char *buf,
			     size_t buf_len, unsigned long timeout,
			     double s_rate, long npoints)
{
	long returned_bytes;

    vxi11_dso7054::agilent_scope_set_for_capture(s_rate, npoints, timeout);
	returned_bytes = vxi11_dso7054::agilent_scope_get_data(chan, buf, buf_len, timeout);
    vxi11_dso7054::agilent_scope_set_for_auto();
	return returned_bytes;
}

/* The following function requests the preample information for the selected waveform source. The preamble data contains information concerning the vertical and horizontal scaling of the data of the correspongding channel
 */ 

int vxi11_dso7054::agilent_scope_get_preamble(char *buf, size_t buf_len)
{
	int ret;
	long bytes_returned;

	ret = vxi11_send_printf(clink, ":WAV:PRE?");
	if (ret < 0) {
		printf("error, could not send :WAV:PRE? cmd, quitting...\n");
		return ret;
	}

	bytes_returned = vxi11_receive(clink, buf, buf_len);

	return (int)bytes_returned;
}

/* Sets the number of averages. If no_averages <= 0 then the averaging is turned
 * off; otherwise the no_averages is set and averaging is turned on. No checking
 * is done for limits. If you enter a number greater than the scope it able to 
 * cope with, it (from experience so far) sets the number of averages to its
 * maximum capability. */
int vxi11_dso7054::agilent_scope_set_averages(int no_averages)
{
	if (no_averages <= 0) {
		return vxi11_send_printf(clink, ":MTES:AVER 0");
	} else {
		vxi11_send_printf(clink, ":ACQ:COUNT %d", no_averages);
		return vxi11_send_printf(clink, ":MTES:AVER 1");
	}
}

/* Gets the number of averages. If acq:aver==0 then returns 0, otherwise
 * returns the actual no of averages. */
long vxi11_dso7054::agilent_scope_get_averages()
{
	long result;

	result = vxi11_obtain_long_value(clink, ":MTES:AVER?");
	if (result == 0)
		return 0;
	result = vxi11_obtain_long_value(clink, ":ACQ:COUNT?");
	return result;
}

/* Get sample rate. Trivial, but used quite often, so worth a little wrapper fn */
double vxi11_dso7054::agilent_scope_get_sample_rate()
{
	return vxi11_obtain_double_value(clink, ":ACQ:SRAT?");
}

/* This function, agilent_write_wfi_file(), saves useful (to us!)
 * information about the waveforms. This is NOT the full set of
 * "preamble" data - things like the data and other bits of info you may find
 * useful are not included, and there are extra elements like the number of
 * waveforms acquired. 
 *
 * It is up to the user to ensure that the scope is in the same condition as
 * when the data was taken before running this function, otherwise the values
 * returned may not reflect those during your data capture; ie if you run
 * agilent_set_for_capture() before you grab the data, then either call that
 * function again with the same values, or, run this function straight after
 * you've acquired your data.
 *
 * Actually there are two agilent_write_wfi_file() functions; the first does
 * a :DIG and finds out how many bytes of data will be collected. This may
 * be useful if you're only capturing one trace; you might as well call this
 * function at the start, find out how many bytes you need for your data
 * array, and write the wfi file at the same time.
 * 
 * But maybe you won't know how many traces you're taking until the end, in
 * which case you'll want to write the wfi file at the end. In that case,
 * you should already know how many bytes per trace there are, and have 
 * also probably recently done a :DIG to capture your data; hence the 
 * second agilent_write_wfi_file() function. */
long vxi11_dso7054::agilent_scope_write_wfi_file(char *wfiname, char chan,
			    char *captured_by, int no_of_traces,
			    unsigned long timeout)
{
	long no_of_bytes;

	/* First we'll calculate the number of bytes. This involves doing a :DIG
	 * command, which (depending on how your scope's set up) may take a fraction
	 * of a second, or a very long time. */
	no_of_bytes = vxi11_dso7054::agilent_scope_calculate_no_of_bytes(chan, timeout);

	/* Now we'll pass this to the _real_ function that does all the work 
	 * (no_of_bytes will stay the same unless there is trouble writing the file) */
	no_of_bytes =
	    vxi11_dso7054::agilent_scope_write_wfi_file(wfiname, no_of_bytes, captured_by,
				   no_of_traces, timeout);

	return no_of_bytes;
}

long vxi11_dso7054::agilent_scope_write_wfi_file(char *wfiname, long no_of_bytes, char *captured_by,  int no_of_traces, unsigned long timeout)
{
	FILE *wfi;
	double vgain, voffset, hinterval, hoffset;
	int ret;

	/* Now find the other info we need to write the wfi-file */
	/* (those paying attention will notice some repetition here,
	 * it's just a bit more logical this way) */
	hinterval = vxi11_obtain_double_value_timeout(clink, ":WAV:XINC?", timeout);
	hoffset = vxi11_obtain_double_value(clink, ":WAV:XORIGIN?");
	vgain = vxi11_obtain_double_value(clink, ":WAV:YINC?");
	voffset = vxi11_obtain_double_value(clink, ":WAV:YORIGIN?");
    
	wfi = fopen(wfiname, "w");
	if (wfi > 0) {
		fprintf(wfi, "%% %s\n", wfiname);
		fprintf(wfi, "%% Waveform captured using %s\n\n", captured_by);
		fprintf(wfi, "%% Number of bytes:\n%d\n\n", no_of_bytes);
		fprintf(wfi, "%% Vertical gain:\n%g\n\n", vgain);
		fprintf(wfi, "%% Vertical offset:\n%g\n\n", -voffset);
		fprintf(wfi, "%% Horizontal interval:\n%g\n\n", hinterval);
		fprintf(wfi, "%% Horizontal offset:\n%g\n\n", hoffset);
		fprintf(wfi, "%% Number of traces:\n%d\n\n", no_of_traces);
		fprintf(wfi, "%% Number of bytes per data-point:\n%d\n\n", 2);	/* always 2 on Agilent scopes */
		fprintf(wfi,
			"%% Keep all datapoints (0 or missing knocks off 1 point, legacy lecroy):\n%d\n\n",
			1);
		fclose(wfi);
	} else {
		printf
		    ("error: agilent_write_wfi_file: could not open %s for writing\n",
		     wfiname);
		return -1;
	}
	return no_of_bytes;
}

void vxi11_dso7054::agilent_scope_get_information(char chan, double & vpp, double & vmax, double & vmin)
{
    char source[20];
    
    vxi11_dso7054::agilent_scope_channel_str(chan, source);
 
    char vpp_cmd[50];  
    sprintf(vpp_cmd, ":MEAS:VPP? %s", source); 
   
    char vmax_cmd[50];  
    sprintf(vmax_cmd, ":MEAS:VMAX? %s", source); 

    char vmin_cmd[50];  
    sprintf(vmin_cmd, ":MEAS:VMIN? %s", source); 

    vpp = vxi11_obtain_double_value(clink, vpp_cmd);
    vmax = vxi11_obtain_double_value(clink, vmax_cmd);
    vmin = vxi11_obtain_double_value(clink, vmin_cmd);
}
