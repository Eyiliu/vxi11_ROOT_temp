/* tek_vxi11.c
 * Copyright (C) 2006-2007 Steve D. Sharples
 *
 * User library of useful functions for talking to Tektronix instruments
 * (e.g. scopes and AFG's) using the VXI11 protocol, for Linux. You will also
 * need the vxi11.tar.gz source, currently available from:
 * http://optics.eee.nottingham.ac.uk/vxi11/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;  version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The author's email address is steve.sharples@nottingham.ac.uk
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>


#ifndef round
#define round(a) floor(a+0.5f)
#endif

#include "tek_vxi11.hh"

/*****************************************************************************
 * Generic Tektronix functions, suitable for all devices                     *
 *****************************************************************************/

/* This really is just a wrapper. Only here because folk might be uncomfortable
 * using commands from the vxi11_user library directly! */
vxi11_mso5000::vxi11_mso5000(const std::string ip_str)
{
  ip = ip_str;
  vxi11_open_device(&clink, ip.c_str(), NULL);
  tek_scope_init();
}

/* Again, just a wrapper */
vxi11_mso5000::~vxi11_mso5000()
{
  vxi11_close_device(clink, ip.c_str());
}

int vxi11_mso5000::tek_scope_select_channel(int ch, int on){
 
  if(on == 0){
    return vxi11_send_printf(clink, ":SEL:CH%ld OFF", ch);
  }
  else{
    return vxi11_send_printf(clink, ":SEL:CH%ld ON", ch);
  }
  
}

/* Set up some fundamental settings for data transfer. It's possible
 * (although not certain) that some or all of these would be reset after
 * a system reset. It's a very tiny overhead right at the beginning of your
 * acquisition that's performed just once. */
int vxi11_mso5000::tek_scope_init()
{
	int ret;
	ret = vxi11_send_printf(clink, ":HEADER 0");	/* no headers in replies */
	if (ret < 0) {
		printf
		    ("error in tek_scope_init, could not send command ':HEADER 0'\n");
		return ret;
	}
	vxi11_send_printf(clink, ":DATA:WIDTH 1");	/* n bytes per data point*/
	vxi11_send_printf(clink, ":DATA:ENCDG SRIBINARY");	/*LSB little endian, signed */
	return 0;
}



/* Makes sure that the number of points we get accurately reflects what's on
 * the screen for the given sample rate. At least that's the aim.
 * If the user has asked to "clear the sweeps", set to single sequence mode. */
long vxi11_mso5000::tek_scope_set_for_capture( int clear_sweeps,
			       unsigned long timeout)
{
	long value, no_bytes;

	/* If we're not "clearing the sweeps" every time, then we need to be
	 * in RUNSTOP mode... otherwise it's just going to grab the same data
	 * over and over again. (If it's a TDS3000, then we've already done
	 * this anyway in the pratting around waiting for XINCR to update). */
	if (clear_sweeps == 0) {
	  vxi11_send_printf(clink, "ACQUIRE:STATE 0");	
	  //RJS removed the runsrop command and changed STATE 1 to STATE 0 to 
	  //get segmented noclsw to work correctly. 
	  //it was breaking repeated runs of clsw and noclsw
	  value = vxi11_obtain_long_value_timeout(clink, "*OPC?", timeout);	
	  //hopefully this wont break anything else!
	}
	
	no_bytes = tek_scope_calculate_no_of_bytes(timeout);

	if (clear_sweeps == 1) {
		vxi11_send_printf(clink, "ACQUIRE:STOPAFTER SEQUENCE");
	}

	return no_bytes;
}



/* Asks the scope for the number of points in the waveform, multiplies by 2
 * (always use 2 bytes per point). This function also sets the DATA:START
 * and DATA:STOP arguments, so that, when in future a CURVE? request is sent,
 * only the data that is displayed on the scope screen is returned, rather
 * than the entire acquisition buffer. This is a PERSONAL PREFERENCE, and is
 * just the way we like to work, ie we set the timebase up so that we can see
 * the signals we are interested in; when we grab the data, we expect the same
 * amount of time that is displayed on the scope screen. Although it doesn't
 * make _acquisition_ any faster (the scope may be acquiring more points than
 * we're interested in), it does reduce bandwidth over LAN. It's mainly so
 * that we get the data we can see on the screen and nothing else, though. */
long vxi11_mso5000::tek_scope_calculate_no_of_bytes( unsigned long timeout)
{
	long no_acq_points;
	long no_points;
	double sample_rate;
	long start, stop;
	double hor_scale;

	no_acq_points = vxi11_obtain_long_value(clink, "HOR:RECORD?");
	hor_scale = vxi11_obtain_double_value(clink, "HOR:MAIN:SCALE?");

	sample_rate = vxi11_obtain_double_value(clink, "HOR:MAIN:SAMPLERATE?");
	no_points = (long)round(sample_rate * 10 * hor_scale);

	start = ((no_acq_points - no_points) / 2) + 1;
	stop = ((no_acq_points + no_points) / 2);
	/* set number of points to receive to be equal to the record length */
	vxi11_send_printf(clink, "DATA:START %ld", start);
	vxi11_send_printf(clink, "DATA:STOP %ld", stop);

	printf("no_acq_points = %ld, no_points = %ld\n",no_acq_points, no_points);
	printf("start = %ld, stop = %ld\n",start, stop);

	long width;
	width = vxi11_obtain_long_value(clink, ":DATA:WIDTH?");	
	return width * no_points;

}

/* Grabs data from the scope */
long vxi11_mso5000::tek_scope_get_data( char *source, int clear_sweeps,
			char *buf, size_t len, unsigned long timeout)
{
	int ret;
	long bytes_returned;
	long opc_value;

	/* set the source channel */
	ret = vxi11_send_printf(clink, "DATA:SOURCE %s", source);
	if (ret < 0) {
		printf("error, could not send DATA SOURCE cmd, quitting...\n");
		return ret;
	}

	/* Do we have to "clear sweeps" ie wait for averaging etc? */
	if (clear_sweeps == 1) {
		/* This is the equivalent of pressing the "Single Seq" button
		 * on the front of the scope */
		vxi11_send_printf(clink, "ACQUIRE:STATE 1");
		/* This request will not return ANYTHING until the acquisition
		 * is complete (OPC? = OPeration Complete?). It's up to the 
		 * user to supply a long enough timeout. */
		opc_value = vxi11_obtain_long_value_timeout(clink, "*OPC?", timeout);
		if (opc_value != 1) {
			printf
			    ("OPC? request returned %ld, (should be 1), maybe you\nneed a longer timeout?\n",
			     opc_value);
			printf("Not grabbing any data, returning -1\n");
			return -1;
		}
	}
	/* ask for the data, and receive it */
	vxi11_send_printf(clink, "CURVE?");
	bytes_returned = vxi11_receive_data_block(clink, buf, len, timeout);

	return bytes_returned;
}


long vxi11_mso5000::mytek_scope_get_data( std::string source, int clear_sweeps,
			char *buf, size_t len, unsigned long timeout)
{
	int ret;
	long bytes_returned;
	long opc_value;
	
	std::cout<<"goto mytek_scope_get_data()"<<std::endl;
	/* set the source channel */
	ret = vxi11_send_printf(clink, "DATA:SOURCE %s", source.c_str());
	if (ret < 0) {
		printf("error, could not send DATA SOURCE cmd, quitting...\n");
		return ret;
	}

	/* Do we have to "clear sweeps" ie wait for averaging etc? */
	if (clear_sweeps == 1) {
		/* This is the equivalent of pressing the "Single Seq" button
		 * on the front of the scope */
	  	std::cout<<  "sending ACQUIRE:STATE 1"<<std::endl;
		vxi11_send_printf(clink, "ACQUIRE:STATE 1");
		/* This request will not return ANYTHING until the acquisition
		 * is complete (OPC? = OPeration Complete?). It's up to the 
		 * user to supply a long enough timeout. */
		opc_value = vxi11_obtain_long_value_timeout(clink, "*OPC?", timeout);
		if (opc_value != 1) {
			printf
			    ("OPC? request returned %ld, (should be 1), maybe you\nneed a longer timeout?\n",
			     opc_value);
			printf("Not grabbing any data, returning -1\n");
			return -1;
		}
	}
	/* ask for the data, and receive it */
	std::cout<<  "sending CURVE?"<<std::endl;
	vxi11_send_printf(clink, "CURVE?");
	bytes_returned = vxi11_receive_timeout(clink, buf, len, timeout);
	std::cout<<"vxi11_receive_timeou return "<<bytes_returned<<std::endl;
	if (bytes_returned < 0) {
	  return bytes_returned;
	}
	if (buf[0] != '#') {
	  printf("mso5000::mygetdata: error: data block does not begin with '#'\n");
	  printf("First 20 characters received were: '");
	  for (int l = 0; l < 20; l++) {
	    printf("%c", buf[l]);
	  }
	  printf("'\n");
	  return -3;
	}

	return bytes_returned;
}



void vxi11_mso5000::tek_scope_set_for_auto()
{
	vxi11_send_printf(clink, "ACQ:STOPAFTER RUNSTOP;:ACQ:STATE 1");
}

/* Sets the number of averages. If passes a number <= 1, will set the scope to
 * SAMPLE mode (ie no averaging). Note that the number will be rounded up or
 * down to the nearest factor of 2, or down to the maximum. */
/* Sets the number of averages. Actually it's a bit cleverer than that, and
 * takes a number based on the acquisition mode, namely:
 * num > 1  == average mode, num indicates no of averages
 * num = 1  == hires mode
 * num = 0  == sample mode
 * num = -1 == peak detect mode
 * num < -1 == envelope mode, (-num) indicates no of envelopes (3000 series
 * only; 4000 series only has infinite no of envelopes).
 * This fn can be used in combination with tek_scope_get_averages which,
 * in combination with this fn, can record the acquisition state and
 * return it to the same. */
int vxi11_mso5000::tek_scope_set_averages( int no_averages)
{
	if (no_averages == 0) {
		return vxi11_send_printf(clink, "ACQUIRE:MODE SAMPLE");
	}
	if (no_averages == 1) {
		return vxi11_send_printf(clink, "ACQUIRE:MODE HIRES");
	}
	if (no_averages == -1) {
		return vxi11_send_printf(clink, "ACQUIRE:MODE PEAKDETECT");
	}
	if (no_averages > 1) {
		vxi11_send_printf(clink, "ACQUIRE:NUMAVG %d", no_averages);
		return vxi11_send_printf(clink, "ACQUIRE:MODE AVERAGE");
	}
	if (no_averages < -1) {
		vxi11_send(clink, "ACQUIRE:NUMENV %d", -no_averages);
		return vxi11_send_printf(clink, "ACQUIRE:MODE ENVELOPE");
	}

	return 1;
}

/* Gets the number of averages. Actually it's a bit cleverer than that, and
 * returns a number based on the acquisition mode, namely:
 * result > 1  == average mode, result indicates no of averages
 * result = 1  == hires mode (4000 series only)
 * result = 0  == sample mode
 * result = -1 == peak detect mode
 * result < -1 == envelope mode, (-result) indicates no of envelopes
 * (3000 series) or -2 (4000 series, this does not have a "no of envelopes"
 * value)
 * This fn can be used in combination with tek_scope_set_averages which,
 * in combination with this fn, can record the acquisition state and
 * return it to the same. */
int vxi11_mso5000::tek_scope_get_averages()
{
	char buf[256];
	long result;
	vxi11_send_and_receive(clink, "ACQUIRE:MODE?", buf, 256,
			       VXI11_READ_TIMEOUT);
	/* Peak detect mode, return -1 */
	if (strncmp("PEA", buf, 3) == 0) {
		return -1;
	}
	/* Sample mode */
	if (strncmp("SAM", buf, 3) == 0) {
		return 0;
	}
	/* Average mode */
	if (strncmp("HIR", buf, 3) == 0) {
		return 1;
	}
	/* Average mode */
	if (strncmp("AVE", buf, 3) == 0) {
		result = vxi11_obtain_long_value(clink, "ACQUIRE:NUMAVG?");
		return (int)result;
	}
	/* Envelope mode */
	if (strncmp("ENV", buf, 3) == 0) {
		vxi11_send_and_receive(clink, "ACQUIRE:NUMENV?", buf, 256,
				       VXI11_READ_TIMEOUT);
		/* If you query ACQ:NUMENV? on a 4000 series, it returns "INFI".
		 * This is not a documented feature, we just have to hope that 
		 * it remains this way. */
		if (strncmp("INF", buf, 3) == 0) {
			return -2;
		} else {
			result = strtol(buf, (char **)NULL, 10);
			return (int)-result;
		}
	}

	return 1;
}

/* Returns the actual number of points that will be returned, based on
 * DATA:START and DATA:STOP */
long vxi11_mso5000::tek_scope_get_no_points()
{
	long start, stop, no_points;

	start = vxi11_obtain_long_value(clink, "DATA:START?");
	stop = vxi11_obtain_long_value(clink, "DATA:STOP?");
	no_points = (stop - start) + 1;
	return no_points;
}

/* Sets the "record length." Here's where Tek differs from Agilent and LeCroy.
 * Supposedly (at least on the 4000 series scopes) you can set the sample
 * rate using "HOR:MAIN:SAMPLERATE" but I've never managed to get this to do
 * anything. The only way you can influence the sample rate is by changing the
 * record length. Depending on the timebase, the number of samples you
 * actually get returned will be less than or equal to the record length you
 * ask for.
 * You cannot ask for any arbitrary record length, either. Valid record
 * lengths are, at time of writing:
 * - TDS3000 series: 500 or 10,000
 * - DPO/MSO4000 series: 1000, 10,000, 100,000, 1,000,000 or 10,000,000
 * This function requests whatever number of points it is passed. It then
 * asks the scope what the record length actually is, and returns this 
 * value. */
long vxi11_mso5000::tek_scope_set_record_length(long record_length)
{
	vxi11_send_printf(clink, "HOR:RECORDLENGTH %ld", record_length);

	return vxi11_obtain_long_value(clink, "HOR:RECORDLENGTH?");
}

/* Returns the sample rate, based on 1/XINCR */
double vxi11_mso5000::tek_scope_get_sample_rate()
{
	double s_rate;

	s_rate = vxi11_obtain_double_value(clink, "HOR:MAIN:SAMPLERATE?");
	return s_rate;
}


// double vxi11_mso5000::mytek_scope_get_point_xinterval(){
//   return vxi11_obtain_double_value(clink, "WFMO:XINCR")

// }


double vxi11_mso5000::mytek_scope_get_vol_point(int ch){
  vxi11_send_printf(clink, ":DATA:SOURCE  CH%ld", ch);
  return vxi11_obtain_double_value(clink, "WFMO:YMULT?");


}

long vxi11_mso5000::mytek_scope_get_yoffset_point(int ch){
  vxi11_send_printf(clink, ":DATA:SOURCE  CH%ld", ch);
  return (long)round(vxi11_obtain_double_value(clink, "WFMO:YOFF?"));

}

