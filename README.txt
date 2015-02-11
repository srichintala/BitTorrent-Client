Project 1: BitTorrent Client Implementation
-----------------------
Name: Mrunal M Pagnis
uname: mmpagnis

Name: Sri Laxmi Chintala
uname: chintals

*************************************

Contents:
---------

1. Introduction
2. Files used
3. Description of code
4. Implementation

*************************************
1. Introduction
---------------

BitTorrent is a peer-to-peer file sharing protocol develop by Bram Cohen. This project
implements a core subset of the BitTorrent protocol by implementing a BitTorrentclient. 
In this, BitTorrent client will take a pre-selected list of peers as a command line 
argument. A BitTorrent client is the application run by down-loaders/up-loaders that 
exchanges pieces of a file.

*************************************
2. Files used
-------------

bt_client.c : 					This is the main portion of the client.

bt_setup.[c|h] : 				This is a C library that provides functionality for parsing 
								command line arguments and peer strings.
								
bt_lib.[c|h] : 					This is the core BitTorrent client library and header file. 
								It contains relevant structures for messages and arguments to 
								BitTorrent client.

Files used for testing:
-----------------------
file.txt.torrent
songs.torrent
moby_dick.txt.torrent 
download.mp3.torrent 		

*************************************
3. Description of code
----------------------
In this project, the seeder and leecher functionality is implemented in different functions.

Firstly, a metainfo file(bencoded dictionary with keys and values) is parsed and the results 
of parsing the file by listing the length, name, piece length of the file and number of pieces
are displayed in the verbose output.

Leecher functionality is implemented in leecher() in which a request for connection is made and
handshake message is sent to the seeder. For the handshake to be sent info_hash(SHA1 hash of the 
bencoded info value in the metafile) and peer_ID(for identifying the client) should be computed. 
These values are computed in cal_info_hash() and calc_id(). The handshake to be generated is 
symmetric and is same for both sides. After the handshake is successful, messages are sent 
and received for communicating. The different types of messages used are defined in lib.c file.

Seeder will first generate the bitfield message with the available pieces, sends the message, and
then it receives the interested message from leecher. Leecher will now request the piece it want 
with respect to the bitfield. Also, it can request pieces in any order. Sending request and storing 
the received piece is implemented in sendRequest(). After sending the request, seeder receives the 
request, checks for the type of message and sends the piece to the leecher which is implemented in 
sendPiece(). Filling the piece is implemented is fillPiece(). When the leecher receives the requested 
piece it is stored and is implemented in storePiece(). For every piece that leecher receives, it notifies
that it has that piece.

*****************************************
4. Implementation
-----------------

This project is implemented in C language. 

The commands for compiling and running the program are as follows:

Compiling the server program(-lssl is required to compile when openssl/hmac.h library is used
for creating the MAC)

Compile
-------
gcc -lssl bt_client.c bt_setup.c bt_lib.c -o bt_client           

Run the program with the IP address, port and file name(using -b(for binding) and -v(verbose) options)
----------------------
./bt_client -v -lssl -b 127.0.0.1:6500 download.mp3.torrent

After running the seeder in one instance, run leecher in another instance with -p option(for peer) and -s option(save file) as follows:
----------------------
./bt_client -v -lssl -p 127.0.0.1:6500 download.mp3.torrent -s somesong.mp3

Default save file name is output.






















