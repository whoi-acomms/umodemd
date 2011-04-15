#!/bin/bash

# eth2um.sh: script to capture ethernet frames, extract the payload (i.e. the
#            IP header and its payload), and package them up as files for a
#            modified version of umodemd to detect and send to the modem.
#            Take advantage of existing code (tcpdump, umodemd, unix toolset).
#
# Created by Jim Partan, April 2011.
#
# $Revision$
# $LastChangedBy$
# $LastChangedDate$
# $HeadURL$
# $Id$
#


#/usr/local/bin/tcpdump_C1 -xSnvvv -s 1514 -C 1 -W 1000 -w logfile_to_SRC101_ dst 192.168.1.101 &

#mkdir transmitted

while /bin/true; do
    for LOGFILE in logfile_to_SRC101_*; do

	# read from tcpdump/libpcap logfile into TMPFILE
	# possibly apply ROHC header compression, which might involve merging
	# Linux ROHC kernel module and tcpdump/libpcap/bittwiste
	/usr/sbin/tcpdump -xtn -s 1514 -r ${LOGFILE} 2>/dev/null > TMPFILE

	# below here is desperately calling out for a python script

	# extract destination IP address from TMPFILE and put it into the
	# raw_modem file.
	# Need to do some error handling here, in case grepping for IP returns
	# nothing.

        DEST_IPV4_ADDRESS=`grep IP TMPFILE | gawk '{print $4}' | cut -d. -f1,2,3,4`
	if [ -n "$DEST_IPV4_ADDRESS" ]; then

        	echo ${DEST_IPV4_ADDRESS} > ${LOGFILE}.raw_modem

		# extract the packet data (IP header, transport-layer header,
		# payload) from TMPFILE. Get rid of whitespace and newlines.
		# Put into .raw_modem file.

        	grep 0x[0-9]*: TMPFILE | cut -d: -f2-9 | sed 's/[ \t]//g' | tr -d '\n' | tr -d '\r' >> ${LOGFILE}.raw_modem

		# convert ${LOGFILE}.modem to ${LOGFILE}.nmea_modem:
        	# a.) Use Routing Table to convert DEST_IPV4_ADDRESS to
		#     next-hop SRC address.
		# b.) Add CCL IP_Payload type.
		# c.) If Path MTU Discovery worked, then TMPFILE data should
		#     fit into one frame.
		#   Generate legacy $CCCYC/$CCTXD commands.
		#   Write ${LOGFILE}.nmea_modem
		# A modified umodemd would then monitor a given directory for
		# new *.nmea_modem files, and move them into the
		# transmitted/ directory/

		mv ${LOGFILE}.raw_modem transmitted/
	fi

	mv ${LOGFILE} transmitted/

    done
done

