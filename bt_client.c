#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <openssl/sha.h>
#include "bt_lib.h"
#include "bt_setup.h"

char **hash = NULL;

/*randomly generate piece index to be requested*/
int RandomPiece(int history[],int n_pieces)
{
	int r=0;
	srand(time(NULL));
	while(1)
	{
		r = rand()%n_pieces;
		if(history[r] == -1)
		break;
	}
	return r;
}
/*reading the torrent file and parsing it*/
int read_file(bt_args_t *bt_args,bt_info_t *info)
{
	
	int fileSize=0;
	int num_pieces=0;
	int piece_len=0;
		
	FILE *fp = NULL;
	char *TorrentFileContents=NULL;
	int TorrentFileSize=0;
	
	/*opening the file*/
	fp = fopen(bt_args->torrent_file,"rb");
	if(fp==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile does not exist");
		}
		return -1;
	}
	
	/*determining the file size of torrent file*/
	fseek(fp,0L,SEEK_END);
	TorrentFileSize = ftell(fp);
	/*if(bt_args->verbose)
	{
		printf("\nFile size is %d",TorrentFileSize);
	}*/
	TorrentFileContents = malloc(sizeof(char)*TorrentFileSize);
	if(TorrentFileContents==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nOut of memory");
		}
		fclose(fp);
		return -1;
	}
	memset(TorrentFileContents,0,sizeof(TorrentFileContents));
	
	/*read the torrent file*/
	rewind(fp);
	int r = fread(TorrentFileContents,1,TorrentFileSize,fp);
	if(r!=TorrentFileSize)
	{
		if(bt_args->verbose)
		{
			printf("\nError in reading file. No.of bytes read = %d",r);
		}
		free(TorrentFileContents);
		fclose(fp);
		return -1;
	}
	
	/*Parse the file*/
	
	char *item = NULL;
	char *temp = NULL;
	
	/*searching for 'info' in torrent file*/
	
	item=strtok(TorrentFileContents, ":");
	if(item==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(TorrentFileContents);
		fclose(fp);
		return -1;
	}
	
	
	while(item)
	{
		if((strlen(item)>4) && (temp=strstr(item, "info"))!=NULL)
		{
			break;
		}
		item=strtok(NULL, ":");
	}
	if(item==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile corrupted");
		}
		free(TorrentFileContents);
		fclose(fp);
		return -1;
	}
	
	/*determining the length of file */
	int i=0,j=0;
	char *length_of_file = malloc(sizeof(char)*30);
	if(length_of_file==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nOut of memory");
		}
		fclose(fp);
		return -1;
	}

	memset(length_of_file,0,sizeof(length_of_file));	
	item=strtok(NULL,":");
	if(item==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(length_of_file);
		fclose(fp);
		return -1;
	}
	if((strlen(item)>5) && (temp=strstr(item, "length"))!=NULL)
	{
		temp = strrchr(item,'i');
	if(temp == NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(length_of_file);
		fclose(fp);
		return -1;
	}
		for(i=1;(*(temp+i))!='e';i++)
		{
			*(length_of_file + i-1) = *(temp + i);
		}
		fileSize = atoi(length_of_file);
	}
	if(fileSize == 0)
	{
		if(bt_args->verbose)
		{
			printf("\nCould not find length of file. Program ends");
		}
		fclose(fp);
		return -1;
	}
	free(length_of_file);
	
	/* determining name of file*/
	int num_of_char=0;
	
	item=strtok(NULL,":");
	if(item == NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		fclose(fp);
		return -1;
	}
	
	if((strlen(item)>4) && (temp=strstr(item, "name"))!=NULL)
	{
		temp=temp+4;
		num_of_char=atoi(temp);
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		fclose(fp);
		return -1;
	}
	
	char fileName[num_of_char];
	memset(fileName,0,sizeof(fileName));
	item = strtok(NULL,":");
	if(item==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		fclose(fp);
		return -1;
	}
	for(i=0;i<num_of_char;i++)
	{
		fileName[i] = *(item + i);		
	}
	fileName[i] = '\0';
	
	/*find the piece length */
	char *length_of_piece = NULL;
	length_of_piece = malloc(sizeof(char)*30);
	if(length_of_piece==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nOut of memory");
		}
		fclose(fp);
		return -1;
	}
	memset(length_of_piece,0,sizeof(length_of_piece));
	item=strtok(NULL,":");
	if(item==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(length_of_piece);
		fclose(fp);
		return -1;
	}
	if((strlen(item)>12) && (temp=strstr(item, "piece length"))!=NULL)
	{
		temp = strrchr(item,'i');
		if(temp == NULL)
		{
			if(bt_args->verbose)
			{
				printf("\nFile contents corrupted");
			}
			free(length_of_piece);
			fclose(fp);
			return -1;
		}
		for(i=1;(*(temp+i))!='e';i++)
		{
			*(length_of_piece + i-1) = *(temp + i);
		}
		piece_len = atoi(length_of_piece);
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(length_of_piece);
		fclose(fp);
		return -1;
	}
	free(length_of_piece);
		
	/*find number of pieces*/
	int total_piece_hash = 0;
	item=strtok(NULL,":");
	if(item == NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(TorrentFileContents);
		fclose(fp);
		return -1;
	}
	if((strlen(item)>6) && (temp=strstr(item, "pieces"))!=NULL)
	{
		temp=temp+6;
		total_piece_hash=atoi(temp);
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nFile contents corrupted");
		}
		free(TorrentFileContents);
		fclose(fp);
		return -1;
	}
	
	num_pieces = total_piece_hash / 20;
	
	
	/*storing hash of each piece*/
	
	hash = malloc(sizeof(char *)*num_pieces);
	if(hash==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nOut of memory");
		}
		fclose(fp);
		return -1;
	}
	memset(hash,0,sizeof(hash));
	for(i=0;i<num_pieces;i++)
	{
		hash[i] = malloc(sizeof(char)*21);
		memset(hash[i],0,sizeof(hash[i])-1);
	}
	
	free(TorrentFileContents);
	rewind(fp);
	fseek(fp,-(total_piece_hash+2),SEEK_END);
	char totalHash[total_piece_hash];
	fread(totalHash,1,total_piece_hash,fp);
	int k=0;
	for(i=0;i<num_pieces;i++)
	{
		for(j=0;j<20;j++,k++)
		{
			hash[i][j] = totalHash[k];
		}
		hash[i][20]='\0';
	}
	
	/*assigning values to info*/
	info->length = fileSize;
	
	strcpy(info->name,fileName);
	
	info->piece_length = piece_len;
	
	info->num_pieces = num_pieces;

	fclose(fp);
	return 1;
}

/*calculation of  info_hash*/
void cal_hash_info(bt_args_t *bt_args, char *i_hash)
{
	FILE *fp = NULL;
	int TorrentFileSize=0;
	
	/*opening the file*/
	fp = fopen(bt_args->torrent_file,"rb");
	if(fp==NULL)
	{
		if(bt_args->verbose)
		{
			printf("\nFile does not exist");
		}
			fclose(fp);
			return;
	}
	
	/*determining the file size of torrent file*/
	fseek(fp,0L,SEEK_END);
	TorrentFileSize = ftell(fp);
	rewind(fp);
	char TorrentFileContents[TorrentFileSize];
	memset(TorrentFileContents, 0, sizeof(TorrentFileContents));
	int i=0;
	/*reading file*/
	for(i=0;i<TorrentFileSize;i++)
	{
		TorrentFileContents[i] = fgetc(fp);
	}
	
	/*searching for "info"*/
	int info_pos = 0;
	for(i=0;i<TorrentFileSize;i++)
	{
		if(TorrentFileContents[i] == 'i')
		{
			if((i+1)<TorrentFileSize && TorrentFileContents[i+1] == 'n')
			{
				if((i+2)<TorrentFileSize && TorrentFileContents[i+2] == 'f')
				{
					if((i+3)<TorrentFileSize && TorrentFileContents[i+3] == 'o')
					{
						info_pos = i+3;
						break;
					}
				}	
			}
		}
	}
	if(info_pos==0)
	{
		if(bt_args->verbose==1)
			printf("\ncould not find info tag in torrent file. Exiting program");
		fclose(fp);
		return;
	}
	info_pos = info_pos + 2;
	int info_size=0;
	info_size = (TorrentFileSize-2) - info_pos;
	char info_part[info_size];
	memset(info_part, 0, sizeof(info_part));
	for(i=info_pos;i<info_size;i++)
	{
		info_part[i] = TorrentFileContents[i];
	}
	SHA1(info_part, info_size, i_hash);
	fclose(fp);
}

/*reading file from given offset and specified piece 
filling up the piece*/
void fillPiece(int verbose, bt_info_t *info, bt_request_t *bt_req, bt_piece_t *bt_p)
{
	FILE *fp;
	fp = fopen(info->name,"rb");
	if(fp==NULL)
	{
		if(verbose)
		{
			printf("\nFile does not exist");
		}
		return;
	}
	int offset = 0,n_bytes =0;
	offset = (info->piece_length)*(bt_req->index) + bt_req->begin;//piece length * index + begin 
	fseek(fp,offset,SEEK_SET);
	n_bytes = offset + bt_req->length;
	
	bt_p->index = bt_req->index;
	bt_p->begin = bt_req->begin;
	
	if((n_bytes) > (info->length))
	{
		n_bytes = info->length - offset;
		bt_p->length = n_bytes;
		fread(bt_p->piece, 1, n_bytes, fp);
		fclose(fp);
		return;
	}
	
	n_bytes = bt_req->length;
	fread(bt_p->piece, 1, n_bytes, fp);
	bt_p->length = n_bytes;
	fclose(fp);
	return;
}

/*reading piece structure 
storing the piece to file*/
void storePiece(int verbose, bt_info_t *info, bt_piece_t *bt_p, int history[], FILE *fp)
{
	int offset = 0;
	 // no.of pieces already written to file which have index less than current piece
	
	int filesize = 0;
	offset = (bt_p->index * info->piece_length) + bt_p->begin;
	fseek(fp, offset, SEEK_SET);
	fwrite(bt_p->piece, 1, bt_p->length, fp);
	return;
}

/*seeder sends the bitfield with available pieces
receives the interest of leecher*/

void sendBitfield(int verbose,bt_info_t *info, int sock, char *bitf)
{
	int i=0;
	
	bt_msg_t Sendmsg;
	bt_msg_t Recvmsg;
	int sizeRmsg = sizeof(Recvmsg.length) + sizeof(Recvmsg.bt_type);
	char buffer[1024];
	memset(buffer,0,sizeof(buffer));
	
	//fill bitfield
	int bsize = 0;
	bsize = info->num_pieces;
	Sendmsg.payload.bitfiled.size = bsize;
	
	memset(Sendmsg.payload.bitfiled.bitfield, 0, bsize);
	for(i=0;i<bsize;i++)
	{
		Sendmsg.payload.bitfiled.bitfield[i] = '1';
	}
	Sendmsg.payload.bitfiled.bitfield[i] = '\0';
	//fill msg
	Sendmsg.bt_type = BT_BITFILED;
	Sendmsg.length = sizeof(Sendmsg.bt_type) + strlen(Sendmsg.payload.bitfiled.bitfield) + sizeof(Sendmsg.payload.bitfiled.size);
	
	if(verbose)
	{
		printf("\nBitfield %s",Sendmsg.payload.bitfiled.bitfield);
	}
	strcpy(bitf, Sendmsg.payload.bitfiled.bitfield);
	//send msg
	send(sock, &(Sendmsg), sizeof(Sendmsg), 0);
	//receive msg
	recv(sock, buffer, sizeRmsg, 0);
	memcpy(&Recvmsg, buffer, sizeRmsg);
	
	if(Recvmsg.bt_type == BT_INTERSTED)
	{
		if(verbose)
		{
			printf("\nLeecher is interested");
		}
	}
	else
	{
		if(verbose)
		{
			printf("\nLeecher not interested");
		}
	}
}

/*leecher receives the bitfield
sends if leecher is interested in the exchange
assumption: leecher is interested in all the pieces which are available*/
	
void recvBitfield(int verbose, bt_info_t *info, int sock, char *btf)
{
	int i=0;
	char buffer[1024];
	memset(buffer,0,sizeof(buffer));
	
	bt_msg_t Sendmsg;
	bt_msg_t Recvmsg;
	
	int n_pieces = info->num_pieces;
	int sizeRmsg = sizeof(Recvmsg.length) + sizeof(Recvmsg.bt_type) + sizeof(char)*n_pieces + sizeof(Recvmsg.payload.bitfiled.size);
	
	/*receive bitfield*/	
	memset(Recvmsg.payload.bitfiled.bitfield, 0 , sizeof(Recvmsg.payload.bitfiled.bitfield));
	recv(sock, buffer, sizeRmsg, 0);
	memcpy(&Recvmsg, buffer, sizeRmsg);
	
	if(Recvmsg.bt_type != BT_BITFILED)
	{
		if(verbose)
		{
			printf("\n***Bit Torrent protocol error. Expecting bitfield***");
		}
		return;
	}
	else
	{
		if(verbose)
		{
			printf("\nbitfield Received");
		}
	}
	if(verbose)
	{
		printf("\nbitfield %s",Recvmsg.payload.bitfiled.bitfield);
	}
	
	strcpy(btf,Recvmsg.payload.bitfiled.bitfield);
	
	/*sending interested*/
	
	Sendmsg.length = sizeof(Sendmsg.bt_type);
	Sendmsg.bt_type = BT_INTERSTED;
	i = send(sock,&Sendmsg, sizeof(Sendmsg), 0);
	if(i<0)
	{
		if(verbose)
		{
			printf("\n could not send interested message. try again");
		}
		return;
	}
	if(verbose)
		printf("\n interested message sent");
		
	return;
}

/*leecher creating a request sending request to seeder 
receiving piece from seeder
storing the piece to file */

int sendRequest(int verbose, int sock,  char *btf, bt_info_t *info, char f_save[])
{
	bt_info_t bt_i;
	
	bt_i.num_pieces = info->num_pieces;
	bt_i.piece_length = info->piece_length;		
	strcpy(bt_i.name,info->name);
	bt_i.length = info->length;
	
	SHA_CTX c;

	int blen = 0;
	blen = strlen(btf);
	char bitfield[blen];
	memset(bitfield,0,sizeof(bitfield));
	strcpy(bitfield,btf);
	
	
	FILE *fp;
	fp = fopen(f_save,"wb+");
	if(fp==NULL)
	{
		if(verbose)
		{
			printf("\nFile could not be created");
		}
		return;
	}
	 
	int i=0, k=0, j=0, r=0;
	int pieces[blen];
	memset(pieces,0,sizeof(pieces));
	
	int n_blocks=0;
	int type=0;
	int b=0, l=1024;	
	int pctr=0;
	int last_plen = 0;
	int last_n_blocks = 0;
	int last_blen = 0;
	
	char sendBuf[1024];
	memset(sendBuf, 0, 1024);
	char recvBuf[1024];
	memset(recvBuf, 0, 1024);
	char *md;
	md = malloc(sizeof(char)*20);
	//memset(md, 0, 1024);
	if(md == NULL)
	{
		if(verbose)
		{
			printf("\n md null");
		}
		return;
	}
	bt_request_t bt_req;
	bt_piece_t bt_p;
	bt_msg_t msg;
	bt_msg_t Rmsg;
	type = 6;

	pctr=bt_i.piece_length;	
	
	n_blocks=(int)(bt_i.piece_length/l);
	
	int history[bt_i.num_pieces];
	last_plen = bt_i.length - ((bt_i.num_pieces-1) * bt_i.piece_length);
	int temp = 0;
	temp = last_plen;

	while(temp >=0)
	{
		temp = temp - l;
		last_n_blocks++;
	}
	for(i=0;i<blen;i++)
	{
		if(bitfield[i]== '1')
		{
			pieces[i] = 1;
		}
		else
			pieces[i] = 0;
		history[i] = -1;
	}
	
	for(i=0;i<bt_i.num_pieces;i++)
	{
		//calculate random index;
		SHA1_Init(&c);
		r = RandomPiece(history,bt_i.num_pieces);
		if(pieces[r]==1)
		{
			if(r == bt_i.num_pieces - 1)
			{
				n_blocks = last_n_blocks;
			}
			for(k=0,b=0;k<n_blocks; k++, b=b+l) //decrement pctr   
			{
				//create request
				msg.payload.request.index=r;
				msg.payload.request.begin=b;
				msg.payload.request.length=l;
				msg.bt_type = BT_REQUEST;
				msg.length = sizeof(msg.bt_type) + sizeof(msg.payload);
				//send request
				send(sock,(const void*)&(msg),sizeof(msg),0);
				memset(&msg,0,sizeof(msg));
				//recv piece
				do{
					memset(&msg,0,sizeof(msg));
					memset(&recvBuf,0,sizeof(recvBuf));
					recv(sock, recvBuf, sizeof(msg), 0); //use size of structure instead of 1024.
					memcpy(&msg,recvBuf,sizeof(msg));
				
				}while(!(msg.bt_type==7));
				//store piece
				storePiece(verbose, &bt_i, &(msg.payload.piece), history, fp);
				SHA1_Update(&c, msg.payload.piece.piece,msg.payload.piece.length);
				memset(recvBuf,0,sizeof(recvBuf));
				memset(&msg,0,sizeof(msg));
			}
			history[r] = r;
			if(r == bt_i.num_pieces - 1)
			{
				n_blocks = bt_i.piece_length/l;
			}
			msg.bt_type = BT_HAVE;
			msg.payload.have = r;
			msg.length = sizeof(msg.bt_type) + sizeof(msg.payload);
			send(sock,(const void*)&(msg),sizeof(msg),0);
			memset(&msg,0,sizeof(msg));
			SHA1_Final(md, &c);
			
			memset(md,0,sizeof(md));
			memset(&c,0,sizeof(c));
		}				
	}
	printf("\nFile downloaded");
	free(md);
	fclose(fp);
}

/*seeder receiving request for piece
filling the piece with data 
sending the piece*/
int sendPiece(int verbose, int sock,  char *btf, bt_info_t *info)
{
	int i=0, k=0;
	int pieces[strlen(btf)];
	memset(pieces,0,sizeof(pieces));
	int n_blocks=0;
	int type=0;
	int b=0, l=1024;	
	int pctr=0;
	
	char sendBuf[1024];
	memset(sendBuf, 0, 1024);
	char recvBuf[1024];
	memset(recvBuf, 0, 1024);
	
	bt_info_t bt_i;
	memset(&bt_i,0,sizeof(bt_i));
	bt_request_t bt_req;
	memset(&bt_req,0,sizeof(bt_req));
	bt_piece_t bt_p;
	memset(&bt_p,0,sizeof(bt_p));
	bt_msg_t msg;
	memset(&msg,0,sizeof(msg));	
	bt_msg_t Smsg;
	memset(&Smsg,0,sizeof(Smsg));
		
	bcopy(info,&bt_i,sizeof(bt_i));
		
	n_blocks = bt_i.piece_length/l;
		
	type = 7;
		
	for(i=0;i<strlen(btf);i++)
	{
		if(btf[i] == '1')
		{
			pieces[i] = 1;
		}
		else
			pieces[i] = 0;
	}
	/*outer loop for piece*/
	
	while(recv(sock, recvBuf, sizeof(msg),0))
	{
		memset(&msg,0,sizeof(msg));
		//reading request
		memcpy(&msg,recvBuf,sizeof(msg)); 
		
		memset(recvBuf,0,sizeof(recvBuf));
		if(msg.bt_type!=6)
		continue;
		//fill piece
	
		fillPiece(verbose, &bt_i, &(msg.payload.request), &(Smsg.payload.piece));
		
		//sending piece	
		Smsg.bt_type = BT_PIECE;	
		Smsg.length = sizeof(Smsg.bt_type) + sizeof(Smsg.payload);
		send(sock,&Smsg,sizeof(Smsg),0);
		
		memset(&bt_p,0,sizeof(bt_p));
		memset(&Smsg,0,sizeof(Smsg));
	}
		/*inner loop for blocks*/
		//use bt_piece to send;
}

/*invoking seeder*/
void seeder(bt_args_t *bt_args)
{
    int sock_seeder=0, sock_leecher=0;
	struct sockaddr_in seederaddr,leecher;
	int size=0;
	char rcv_handshake[68]={};
	
	bt_info_t info;
	char *btf;
	info.num_pieces = bt_args->bt_info->num_pieces;
	info.piece_length = bt_args->bt_info->piece_length;
	strcpy(info.name,bt_args->bt_info->name);
	info.length = bt_args->bt_info->length;
	btf = malloc(sizeof(char)*info.num_pieces);
	
	sock_seeder=socket(AF_INET, SOCK_STREAM, 0);
	if(bt_args->verbose)
	{
		printf("\nTrying to bind...");
	}
	if(bind(sock_seeder, (struct sockaddr*)&(bt_args->peers[0]->sockaddr), sizeof(bt_args->peers[0]->sockaddr))<0)
	{
		if(bt_args->verbose)
		{
			printf("\nPort address is already bind\n");
		}
		return;
	}
	printf("\nTrying to listen for connections...");
	if(listen(sock_seeder, MAX_CONNECTIONS)<0)
	{
		if(bt_args->verbose)
		{
			printf("\nSeeder could not listen on port %d", bt_args->peers[0]->port);
		}
		return;
	}
	if(bt_args->verbose)
	{
		printf("\nSeeder listening on port %d", bt_args->peers[0]->port);
	}
	size= sizeof(struct sockaddr);
	sock_leecher = accept(sock_seeder, (struct sockaddr*)&leecher,(socklen_t*)&size);
	if(sock_leecher<0)
	{
		if(bt_args->verbose)
		{
			printf("\nCould not accept");
		}
	}
	if(bt_args->verbose)
	{
		printf("\nConnected to client successfully");
	}
	
	/* generate handshake message to be sent*/
	char completeHandShake[68];
	char bt_protocol[20];
	char reserved[8];
	char info_hash[20];// = malloc(sizeof(char)*20);
	char peer_ID[20]={0};
	memset(bt_protocol,0,sizeof(bt_protocol));
	memset(reserved,0,sizeof(reserved));
	memset(info_hash,0,sizeof(info_hash));
	memset(peer_ID,0,sizeof(peer_ID));
	memset(rcv_handshake,0,sizeof(rcv_handshake));
	memset(completeHandShake,0,sizeof(completeHandShake));
	
	int i=0, k=0, j=0, p=0;
	//bt_protocol[0] = (unsigned char) 19; 
	//strcpy((char *) (bt_protocol + 1), "BitTorrent Protocol");
	strcpy(bt_protocol, "xBitTorrent Protocol");
	for(k=0;k<8;k++)
	{
		reserved[k]=0;
	}
	cal_hash_info(bt_args,info_hash);
	for(j=0;j<20;j++)
	{
		peer_ID[j]=bt_args->peers[i]->id[j];
	}
	for(i=0;i<20;i++)
    {
        completeHandShake[i] = bt_protocol[i];
    }
    for(i=20;i<28;i++)
    {
        completeHandShake[i] = '0';
    }
    for(i=28;i<48;i++)
    {
        completeHandShake[i] = info_hash[i-28];
    }
    for(i=48;i<68;i++)
    {
        completeHandShake[i] = peer_ID[i-48];
    }
	p=send(sock_leecher, completeHandShake, 68, 0);
	if(p<0)
	{
		if(bt_args->verbose)
		{
			printf("\nError in sending handshake");
		}
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nHandshake sent successfully");
		}
	}
	p=recv(sock_leecher, rcv_handshake, 68, 0);
	if(p<0)
	{
		if(bt_args->verbose)
		{
			printf("\nError in receiving handshake");
		}
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nHandshake received successfully");
		}
	}
	for(i=28;i<68;i++)
	{
		if(rcv_handshake[i]!=completeHandShake[i])
		{
			if(bt_args->verbose)
			{
				printf("\nHandshake unsuccessful");
			}
			break;
		}
	}
	if(i!=68)
	{
		close(sock_leecher);
		close(sock_seeder);
		return;
	}
	if(bt_args->verbose)
	{
		printf("\nHandshake successful");
	}
	sendBitfield(bt_args->verbose,&info, sock_leecher, btf);
	
	sendPiece(bt_args->verbose, sock_leecher, btf, &info);

	close(sock_leecher);
	close(sock_seeder);
 } 
  
/*invoking leecher*/
void leecher(bt_args_t *bt_args)
{
	bt_info_t info;
	int lsocket,i=0,peer=0;
	char *btf;
	info.num_pieces = bt_args->bt_info->num_pieces;
	info.piece_length = bt_args->bt_info->piece_length;
	strcpy(info.name,bt_args->bt_info->name);
	info.length = bt_args->bt_info->length;
	
	btf = malloc(sizeof(char)*info.num_pieces);
	memset(btf,0,sizeof(btf));
	lsocket = socket(AF_INET,SOCK_STREAM,0);
	for(i=1;i<MAX_CONNECTIONS;i++)
	{
		if(bt_args->peers[i]!=NULL)
		{
			peer = i;
			break;
		}
	}
	if(i==6)
	{
		return;
	}
	struct sockaddr_in leecher = bt_args->peers[peer]->sockaddr;
	if(connect(lsocket,(struct sockaddr*)&leecher,sizeof(leecher))<0)
	{
		if(bt_args->verbose)
		{
			printf("\nError in socket connection");
		}
		close(lsocket);
		return;
	}
		printf("\nYou have connected to server");
	
	/*create handshake message to be sent*/
	
	char completeHandShake[68]={'0'};
	memset(completeHandShake,0,sizeof(completeHandShake));
	char bt_protocol[20];
	memset(bt_protocol,0,sizeof(bt_protocol));
	char reserved[8];
	memset(reserved,0,sizeof(reserved));
	char info_hash[20];
	memset(info_hash,0,sizeof(info_hash));
	char peer_ID[20];
	memset(peer_ID,0,sizeof(peer_ID));
	char get_handshake[68];
	memset(get_handshake,0,sizeof(get_handshake));

	int k=0;
	int j=0, p=0, m=0;
	//bt_protocol[0] = (unsigned char) 19;
	
	//strcpy((char *) (bt_protocol[1]), "BitTorrent Protocol");
	strcpy(bt_protocol, "xBitTorrent Protocol");
	for(k=0;k<8;k++)
	{
		reserved[k]=0;
	}
	cal_hash_info(bt_args,info_hash);
	for(j=0;j<20;j++)
	{
		peer_ID[j]=bt_args->peers[i]->id[j];
	}
	for(k=0;k<20;k++)
	{
		completeHandShake[k] = bt_protocol[k];
	}
	for(k=20;k<28;k++)
	{
		completeHandShake[k] ='0';
	}
	for(k=28;k<48;k++)
	{
		completeHandShake[k] = info_hash[k-28];
	}
	for(k=48;k<68;k++)
	{
		completeHandShake[k] = peer_ID[k-48];
	}
	p=send(lsocket, completeHandShake, 68,0);
	if(p<0)
	{
		if(bt_args->verbose)
		{
			printf("\nError in sending handshake");
		}
	}
	else
	{
		if(bt_args->verbose)
		{
			printf("\nHandshake sent successfully");
		}
	}
	m = recv(lsocket, get_handshake, 68, 0);
	if(m<0)
	{
		if(bt_args->verbose)
		{
			printf("\nError in receiving handshake");
		}
	}
	else
	{	
		if(bt_args->verbose)
		{
			printf("\nHandshake received successfully");
		}
	}
	
	for(i=28;i<68;i++)
	{
		if(get_handshake[i]!=completeHandShake[i])
		{
			if(bt_args->verbose)
			{
				printf("\nHandshake Unsuccessful");
			}
			break;
		}
	}
	
	if(i==68)
	{
		recvBitfield(bt_args->verbose,&info,lsocket,btf);
		sendRequest(bt_args->verbose,lsocket,btf,&info,bt_args->save_file);
		//printf("\nFile name in leecher %s",info.name);
	}	
	close(lsocket);
}

int main (int argc, char * argv[])
{

  bt_args_t bt_args;
  bt_info_t info;
  int i=0,j=0,k=0;

  parse_args(&bt_args, argc, argv);

  if(bt_args.verbose){
    printf("Args:\n");
    for(i=0;i<MAX_CONNECTIONS;i++){
      if(bt_args.peers[i] != NULL)
        print_peer(bt_args.peers[i]);
    } 
  }

  //read and parse the torrent file here
  int x =0;
  x = read_file(&bt_args, &info);
  if(x == -1)
  {
	printf("\nParsing Unsuccessful. Program exits");
	return;
  }
  info.piece_hashes = hash;
	
  bt_args.bt_info = &info;
	
  if(bt_args.verbose){
    // print out the torrent file arguments here
	printf("\nLength of file =%d",bt_args.bt_info->length);
	printf("\nName of file =%s",bt_args.bt_info->name);
	printf("\nLength of piece =%d",bt_args.bt_info->piece_length);
	printf("\nTotal number of pieces =%d",bt_args.bt_info->num_pieces);
	for(i=0;i<info.num_pieces;i++)
	{
			printf("\nHash of Piece %d = %s",i+1, bt_args.bt_info->piece_hashes[i]);
	}
  }
  
  if(bt_args.bind == 1)
  {
    seeder(&bt_args);
  }
  else
    leecher(&bt_args);
  
  for(i=0;i<info.num_pieces;i++)
	free(hash[i]);
	free(hash);
	
  for(i=0;i<MAX_CONNECTIONS;i++)
  {
	if(bt_args.peers[i]!=NULL)
	free(bt_args.peers[i]);
  }
  return 0;
}
 