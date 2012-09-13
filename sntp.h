
struct ntp_packet {
  unsigned char mode : 3;
  unsigned char vn : 3;
  unsigned char li : 2;
  unsigned char stratum;
  char poll;
  char precision;
  unsigned long root_delay;
  unsigned long root_dispersion;
  unsigned long reference_identifier;
  unsigned long reference_timestamp_secs;
  unsigned long reference_timestamp_fraq;
  unsigned long originate_timestamp_secs;
  unsigned long originate_timestamp_fraq;
  unsigned long receive_timestamp_secs;
  unsigned long receive_timestamp_fraq;
  unsigned long transmit_timestamp_secs;
  unsigned long transmit_timestamp_fraq;
};

struct ntp_timestamp {
  unsigned long secs;
  unsigned long msecs;
};


SYSTEMTIME SecsToSystemTime(unsigned long, int);
SYSTEMTIME NTPPacketToSystemTime(unsigned long,unsigned long,int);
void printpkt_timestamp(SYSTEMTIME);
void printpkt(struct ntp_packet*);
void prepare_packet(unsigned long*, unsigned long*, SYSTEMTIME*);
int sntp_connect(char*);
