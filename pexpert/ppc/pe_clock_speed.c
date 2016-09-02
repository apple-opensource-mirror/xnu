/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *  pe_clock_speed.c - Determine the best guess for the processor and bus
 *                     speed buy using the values returned by run_clock_test.
 *
 *  (c) Apple Computer, Inc. 1998-2000
 *
 *  Writen by:   Josh de Cesare
 *
 */

#include <pexpert/pexpert.h>

#include <ppc/machine_routines.h>

// prototypes
extern void pe_run_clock_test(void *tmp);
void pe_do_clock_test(unsigned int via_addr,
		      int num_speeds, unsigned long *speed_list);

// Threshold for bus speed matches.
#define kMaxFreqDiff  (30000)

// This is the structure for the data that get passed to pe_run_clock_test.
struct clock_test_data {
  unsigned int via_addr;
  unsigned int via_ticks;
  unsigned int dec_ticks;
};

// glocal variables to simplify some stuff.
static long bus_freq_num, bus_freq_den, cpu_pll;

// PE_Determine_Clock_Speeds is called by the via driver in IOKit
// It uses the numbers generated by pe_do_clock_test and reports
// the cleaned up values to the rest of the OS.
void PE_Determine_Clock_Speeds(unsigned int via_addr, int num_speeds,
			       unsigned long *speed_list)
{
  boolean_t          oldLevel;
  unsigned long      tmp_bus_speed, tmp_cpu_speed;
  unsigned long long tmp;
  
  oldLevel = ml_set_interrupts_enabled(FALSE);
  pe_do_clock_test(via_addr, num_speeds, speed_list);
  ml_set_interrupts_enabled(oldLevel);
  
  tmp_bus_speed = bus_freq_num / bus_freq_den;
  tmp = ((unsigned long long)bus_freq_num * cpu_pll) / (bus_freq_den * 2);
  tmp_cpu_speed = (unsigned long)tmp;
  
  // Report the bus clock rate as is.
  gPEClockFrequencyInfo.bus_clock_rate_num = bus_freq_num;
  gPEClockFrequencyInfo.bus_clock_rate_den = bus_freq_den;
  
  // pll multipliers are in halfs so set the denominator to 2.
  gPEClockFrequencyInfo.bus_to_cpu_rate_num = cpu_pll;
  gPEClockFrequencyInfo.bus_to_cpu_rate_den = 2;
  
  // The decrementer rate is one fourth the bus rate.
  gPEClockFrequencyInfo.bus_to_dec_rate_num = 1;
  gPEClockFrequencyInfo.bus_to_dec_rate_den = 4;
  
  // Set the truncated numbers in gPEClockFrequencyInfo.
  gPEClockFrequencyInfo.bus_clock_rate_hz = tmp_bus_speed;
  gPEClockFrequencyInfo.cpu_clock_rate_hz = tmp_cpu_speed;
  gPEClockFrequencyInfo.dec_clock_rate_hz = tmp_bus_speed / 4;
  
  PE_call_timebase_callback();
}

// pe_do_clock_test uses the number from pe_run_clock_test to
// find a best fit guess for the bus speed.
void pe_do_clock_test(unsigned int via_addr,
		      int num_speeds, unsigned long *speed_list)
{
  struct clock_test_data clock_test_data;
  long cnt, diff, raw_cpu_freq, raw_bus_freq, tmp_bus_freq,
    last_bus_freq, tries = 10;
  
  // Save the via addr so the asm part can use it.
  clock_test_data.via_addr = via_addr;
  
  // Keep looping until it matches the last try.
  bus_freq_num = 0;
  do {
    last_bus_freq = bus_freq_num;
    
    // The the asm part to do the real work.
    pe_run_clock_test((void *)&clock_test_data);
    
    // First find the pll mode.  Allow any integer times two.
    cpu_pll = 10000000 / clock_test_data.dec_ticks;
    cpu_pll = (cpu_pll / 2) + (cpu_pll & 1);
    
    // Using 64 bit math figure out the raw bus speed.
    // 0xBF401675E5DULL is 1 / 1.27655us times 2 ^ 24.
    raw_bus_freq = ((0xBF401675E5DULL * clock_test_data.dec_ticks) /
		    clock_test_data.via_ticks) >> 22;
    
    // use the pll mode and the raw bus speed to find the raw cpu speed.
    raw_cpu_freq = raw_bus_freq * cpu_pll / 2;
    
    // Look to see if the bus speed is close to one of the
    // speeds in the table.
    for (cnt = 0; cnt < num_speeds; cnt++) {
      bus_freq_num = speed_list[cnt * 2];
      bus_freq_den = speed_list[cnt * 2 + 1];
      diff = bus_freq_num - raw_bus_freq * bus_freq_den;
      if (diff < 0) diff = -diff;
      
      if (diff < kMaxFreqDiff * bus_freq_den) break;
    }
    if (cnt != num_speeds) continue;
    
    // Look to see if the bus speed is close to n * 0.5 MHz
    tmp_bus_freq = ((raw_bus_freq + 250000) / 500000) * 500000;
    
    diff = tmp_bus_freq - raw_bus_freq;
    if (diff < 0) diff = -diff;
    
    if (diff < kMaxFreqDiff) {
      bus_freq_num = tmp_bus_freq;
      bus_freq_den = 1;
      continue;
    }
    
    // Look to see if the bus speed is close to n * 50/3 MHz
    tmp_bus_freq = ((raw_bus_freq * 3 + 25000000) / 50000000) * 50000000;
    
    diff = tmp_bus_freq - raw_bus_freq * 3;
    if (diff < 0) diff = -diff;
    
    if (diff < kMaxFreqDiff * 3) {
      bus_freq_num = tmp_bus_freq;
      bus_freq_den = 3;
      continue;
    }
    
    // Since all else failed return the raw bus speed
    bus_freq_num = raw_bus_freq;
    bus_freq_den = 1;
  } while ((bus_freq_num != last_bus_freq) && tries--);
}
