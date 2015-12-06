// Arduino UNO connected to a ENC28J60 Ethernet shield 1.1 and two relays
#include "EtherShield.h"

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x25}; 
static uint8_t myip[4] = {192,168,0,125};

#define MYWWWPORT 80
#define BUFFER_SIZE 1024
#define POWER_PIN 5
#define SPEED_PIN 3
int power_state=0;
int speed_state=0;
char command;
static uint8_t buf[BUFFER_SIZE+1];

// The ethernet shield
EtherShield es=EtherShield();

uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
  uint16_t plen;
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<title>Arduino AirCon Controller V1.0</title>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<link rel='stylesheet' href='//netdna.bootstrapcdn.com/bootstrap/3.0.0/css/bootstrap.min.css'>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<script src='//netdna.bootstrapcdn.com/bootstrap/3.0.0/js/bootstrap.min.js'></script>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</head><body>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<div class='container'><h3>EthernetAirCon</h3>"));

  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<div class='row'>"));
  if (power_state == 0){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<form action='/power/on' method='post'><button type='submit' class='btn btn-danger'>POWER IS OFF</button></form>"));  
  }else {
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<form action='/power/off' method='post'><button type='submit' class='btn btn-success'>POWER IS ON</button></form>")); 
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</div>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<div class='row'>"));
  if (speed_state == 0){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<form action='/turbo/on' method='post'><button type='submit' class='btn btn-danger'>TURBO IS OFF</button></form>"));  
  }else {
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<form action='/turbo/off' method='post'><button type='submit' class='btn btn-success'>TURBO IS ON</button></form>"));  
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</div>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<a href='/api'>api</a>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</div>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>"));

  return(plen);
}

uint16_t print_api(uint8_t *buf)
{
  uint16_t plen;
  plen=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n\r\n"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("{'power':'"));
  if (power_state==1){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("on"));
  }else{
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("off"));
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("','"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("'turbo':'"));
  if (speed_state==1){
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("on"));
  }else{
    plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("off"));
  }
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("'}"));
}


void setup(){
  pinMode(POWER_PIN, OUTPUT);
  pinMode(SPEED_PIN, OUTPUT);
  
  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  // init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);
}


void loop(){
  uint16_t plen, dat_p;

  while(1) {
    // read packet, handle ping and wait for a tcp packet:
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

    /* dat_p will be unequal to zero if there is a valid 
     * http get */
    if(dat_p==0){
      // no http request
      continue;
    }

    if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
      dat_p=print_webpage(buf);
      goto SENDTCP;
    }
    else if (strncmp("GET /api ",(char *)&(buf[dat_p]),9)==0){
      dat_p=print_api(buf);
      goto SENDTCP;
    }
    else if (strncmp("POST /power/on ",(char *)&(buf[dat_p]),15)==0){
      power_state=1;
      digitalWrite(POWER_PIN, HIGH);
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n"));
      goto SENDTCP;
    }
    else if (strncmp("POST /power/off ",(char *)&(buf[dat_p]),16)==0){
      power_state=0;
      digitalWrite(POWER_PIN, LOW);
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n"));
      goto SENDTCP;
    }
    else if (strncmp("POST /turbo/on ",(char *)&(buf[dat_p]),15)==0){
      speed_state=1;
      digitalWrite(SPEED_PIN, HIGH);
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n"));
      goto SENDTCP;
    }
    else if (strncmp("POST /turbo/off ",(char *)&(buf[dat_p]),16)==0){
      speed_state=0;
      digitalWrite(SPEED_PIN, LOW);
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n"));
      goto SENDTCP;
    }
    else{
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
      goto SENDTCP;
    }
SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end

  }
}



