// qjackctlStatus.h
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __qjackctlStatus_h
#define __qjackctlStatus_h

// List view statistics item indexes
#define STATUS_SERVER_NAME      0
#define STATUS_SERVER_STATE     1
#define STATUS_DSP_LOAD         2
#define STATUS_SAMPLE_RATE      3
#define STATUS_BUFFER_SIZE      4
#define STATUS_REALTIME         5
#define STATUS_TRANSPORT_STATE  6
#define STATUS_TRANSPORT_TIME   7
#define STATUS_TRANSPORT_BBT    8
#define STATUS_TRANSPORT_BPM    9
#define STATUS_XRUN_COUNT       10
#define STATUS_XRUN_TIME        11
#define STATUS_XRUN_LAST        12
#define STATUS_XRUN_MAX         13
#define STATUS_XRUN_MIN         14
#define STATUS_XRUN_AVG         15
#define STATUS_XRUN_TOTAL       16
#define STATUS_RESET_TIME       17
#define STATUS_MAX_DELAY        18

// (Big)Time display identifiers.
#define DISPLAY_TRANSPORT_TIME  0
#define DISPLAY_TRANSPORT_BBT   1
#define DISPLAY_RESET_TIME      2
#define DISPLAY_XRUN_TIME       3

#endif  // __qjackctlStatus_h

// end of qjackctlStatus.h

