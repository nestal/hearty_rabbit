/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#pragma once

#include "config.hh"
#include <syslog.h>

#ifdef SYSTEMD_FOUND
#include <systemd/sd-journal.h>
#define LOG sd_journal_print
#else
#define LOG syslog
#endif