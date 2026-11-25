#include "greenhouse_server.h"
#include "greenhouse_client.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void through_wire(GreenhouseServer *s, SpiMessage *request, uint32_t now) {
  uint8_t bytes[SPI_PACKET_SIZE]; SpiMessage decoded;
  SpiProtocol_Encode(bytes, request);
  assert(SpiProtocol_Decode(&decoded, bytes));
  GreenhouseServer_Receive(s, &decoded, now);
}
static void sample(GreenhouseServer *s, uint16_t seq, int t, unsigned h, int valid, uint32_t now) {
  SpiMessage m = {0}; m.type = SPI_TYPE_MEASUREMENT; m.sequence = seq;
  m.temperature10 = (int16_t)t; m.humidity10 = (uint16_t)h; m.flags = valid ? SPI_DATA_VALID : 0;
  through_wire(s, &m, now);
}
static void hello(GreenhouseServer *s, uint16_t cookie, uint32_t now) {
  SpiMessage m = {0}; m.type = SPI_TYPE_HELLO; m.sequence = cookie; through_wire(s, &m, now);
}
typedef struct {
  GreenhouseServer server;
  uint8_t armed[SPI_PACKET_SIZE], published[SPI_PACKET_SIZE];
} Peer;
static void peer_init(Peer *p, uint32_t now) {
  GreenhouseServer_Init(&p->server);
  SpiMessage status; GreenhouseServer_Status(&p->server,now,1000,&status);
  SpiProtocol_Encode(p->published,&status);
  memcpy(p->armed,p->published,SPI_PACKET_SIZE);
}
/* Wire boundary model: DMA answers with its immutable snapshot, NSS rearms
 * from the previously published status, then the foreground handles RX. */
static void round_trip(Peer *p, GreenhouseClient *c, uint32_t now, int bad_tx, int bad_rx) {
  SpiMessage request, received, status;
  uint8_t tx[SPI_PACKET_SIZE], rx[SPI_PACKET_SIZE];
  GreenhouseClient_Request(c,&request); SpiProtocol_Encode(tx,&request);
  memcpy(rx,p->armed,SPI_PACKET_SIZE);
  memcpy(p->armed,p->published,SPI_PACKET_SIZE);
  if (bad_tx) tx[6] ^= 1;
  if (SpiProtocol_Decode(&received,tx)) GreenhouseServer_Receive(&p->server,&received,now);
  else p->server.rx_error = 1;
  GreenhouseServer_Status(&p->server,now,p->server.control.open?1500:1000,&status);
  SpiProtocol_Encode(p->published,&status);
  if (bad_rx) rx[7] ^= 1;
  GreenhouseClient_Reply(c,rx,now);
}
int main(void) {
  assert(SpiProtocol_Crc((const uint8_t *)"123456789", 9) == 0x29B1);
  SpiMessage m = {0}, decoded;
  m.type = SPI_TYPE_MEASUREMENT; m.sequence = 1; m.temperature10 = 299;
  m.humidity10 = 550; m.flags = SPI_DATA_VALID;
  uint8_t bytes[SPI_PACKET_SIZE]; SpiProtocol_Encode(bytes, &m);
  const uint8_t expected[] = {0xA5,0x5A,1,0x10,0,1,1,0x2B,2,0x26,1,0,0x92,0x82};
  assert(!memcmp(bytes, expected, sizeof bytes));
  for (unsigned bit = 0; bit < SPI_PACKET_SIZE * 8; ++bit) {
    bytes[bit/8] ^= (uint8_t)(1U << (bit%8));
    assert(!SpiProtocol_Decode(&decoded, bytes));
    bytes[bit/8] ^= (uint8_t)(1U << (bit%8));
  }
  m.temperature10 = -123; SpiProtocol_Encode(bytes, &m);
  assert(SpiProtocol_Decode(&decoded, bytes) && decoded.temperature10 == -123);
  m.humidity10 = 1001; SpiProtocol_Encode(bytes, &m); assert(!SpiProtocol_Decode(&decoded, bytes));
  m.humidity10 = 500; m.temperature10 = 801;
  SpiProtocol_Encode(bytes, &m); assert(!SpiProtocol_Decode(&decoded, bytes));
  m.flags = 0; SpiProtocol_Encode(bytes, &m); assert(SpiProtocol_Decode(&decoded, bytes));
  m.type = 0xEE; SpiProtocol_Encode(bytes, &m); assert(!SpiProtocol_Decode(&decoded, bytes));
  assert(SpiProtocol_IsNewSequence(0,65535));
  assert(!SpiProtocol_IsNewSequence(65535,0));
  assert(!SpiProtocol_IsNewSequence(5,5));
  assert(!SpiProtocol_IsNewSequence(0x8000,0));

  GreenhouseServer s; GreenhouseServer_Init(&s);
  sample(&s,1,250,550,1,0); assert(!s.have_measurement); /* HELLO is mandatory. */
  hello(&s,1234,0);
  GreenhouseServer_Status(&s,0,1000,&decoded);
  assert(decoded.sequence == 1234 && (decoded.flags & SPI_STATUS_SESSION_READY));
  assert(!(decoded.flags & SPI_STATUS_HAVE_MEASUREMENT));
  sample(&s,1,250,550,1,10); assert(!s.control.open && s.control.confirmations == 1);
  sample(&s,1,250,550,1,2200); assert(!s.control.open && s.control.last_good == 10);
  sample(&s,0,250,550,1,2300); assert(!s.control.open && s.sequence == 1);
  sample(&s,2,250,550,1,2400); assert(s.control.open);
  GreenhouseServer_Status(&s,2400,1500,&decoded);
  assert(decoded.sequence == 2 && (decoded.flags & SPI_STATUS_OPEN) && (decoded.flags & SPI_STATUS_DATA_VALID));
  SpiProtocol_Encode(bytes,&decoded); assert(SpiProtocol_Decode(&m,bytes));
  assert(m.pulse_us == 1500 && m.age_seconds == 0);
  /* Equality and the hysteresis band hold the request. */
  sample(&s,3,250,380,1,3000); assert(s.control.open);
  sample(&s,4,250,379,1,4000); assert(s.control.open);
  sample(&s,5,250,379,0,5000); assert(s.control.open && !s.control.confirmations);
  sample(&s,6,250,379,1,6000); assert(s.control.open);
  sample(&s,7,250,379,1,7000); assert(!s.control.open);
  /* Temperature independently opens a dry greenhouse. */
  sample(&s,8,281,100,1,8000); sample(&s,9,281,100,1,9000); assert(s.control.open);
  sample(&s,10,260,100,1,10000); assert(s.control.open);
  sample(&s,11,259,100,1,11000); assert(s.control.open);
  GreenhouseServer_Status(&s,21000,1500,&decoded);
  assert((decoded.flags & SPI_STATUS_STALE) && !(decoded.flags & SPI_STATUS_DATA_VALID));
  assert(s.control.open && !s.control.confirmations);
  /* Polls and retransmissions never refresh the measurement age. */
  SpiMessage poll = {0}; poll.type = SPI_TYPE_POLL;
  through_wire(&s,&poll,22000); sample(&s,11,259,100,1,23000);
  assert(s.control.last_good == 11000 && s.control.open);
  sample(&s,12,259,100,1,24000); assert(s.control.open);
  sample(&s,13,259,100,1,26000); assert(!s.control.open);
  /* Source reboot explicitly resynchronizes the sequence, preserving PWM. */
  hello(&s,555,27000); assert(!s.have_measurement && !s.control.have_good);
  sample(&s,1,250,500,1,28000); sample(&s,2,250,500,1,30000); assert(s.control.open);
  hello(&s,556,31000); assert(s.control.open && !s.have_measurement);
  sample(&s,65535,250,300,1,32000); sample(&s,0,250,300,1,34000); assert(!s.control.open);
  /* Receiver reboot has no session; the master must HELLO again. */
  GreenhouseServer_Init(&s); sample(&s,1,250,500,1,35000); assert(!s.have_measurement);
  hello(&s,557,36000); sample(&s,2,250,500,1,37000); assert(s.have_measurement);
  Greenhouse g; Greenhouse_Init(&g);
  Greenhouse_Sample(&g,250,500,1,UINT32_MAX-1000U);
  assert(!Greenhouse_Stale(&g,500));
  assert(Greenhouse_Stale(&g,10000));
  /* Boundaries for all humidity values: low temperature, two fresh samples. */
  for (unsigned h = 0; h <= 1000; ++h) {
    Greenhouse_Init(&g); Greenhouse_Sample(&g,250,(uint16_t)h,1,0);
    Greenhouse_Sample(&g,250,(uint16_t)h,1,2200);
    assert(g.open == (h > 400));
  }
  /* Exercise both production endpoints with two-frame delayed replies. */
  Peer peer; GreenhouseClient client;
  peer_init(&peer,0); GreenhouseClient_Init(&client,0xAA55);
  for (unsigned i=0;i<8;++i) round_trip(&peer,&client,i*100,0,0);
  assert(client.ready && GreenhouseClient_Online(&client,700));
  GreenhouseClient_Measurement(&client,250,500,1);
  round_trip(&peer,&client,1000,1,0); /* Damaged request is not acknowledged. */
  assert(client.pending && !peer.server.have_measurement);
  round_trip(&peer,&client,1100,0,0); assert(client.pending && peer.server.control.confirmations==1);
  round_trip(&peer,&client,1200,0,1); assert(client.pending);
  for (unsigned i=0;i<5;++i) round_trip(&peer,&client,1300+i*100,0,0);
  assert(!client.pending && client.remote.sequence==1 && !peer.server.control.open);
  assert(peer.server.control.confirmations==1); /* Retries did not count twice. */
  GreenhouseClient_Measurement(&client,250,500,1);
  for (unsigned i=0;i<5;++i) round_trip(&peer,&client,3200+i*100,0,0);
  assert(!client.pending && peer.server.control.open);
  assert((client.remote.flags & SPI_STATUS_OPEN) && client.remote.pulse_us==1500);
  assert(!GreenhouseClient_Online(&client,5000));
  /* Queries stay alive, but cannot keep an old measurement fresh. */
  for (unsigned i=0;i<5;++i) round_trip(&peer,&client,15000+i*100,0,0);
  assert(GreenhouseClient_Online(&client,15400));
  assert(client.remote.flags & SPI_STATUS_STALE);
  assert(peer.server.control.open);
  /* Actuator reset is noticed and the client negotiates again. */
  peer_init(&peer,16000);
  for (unsigned i=0;i<10;++i) round_trip(&peer,&client,16000+i*100,0,0);
  assert(client.ready && !client.pending && peer.server.have_measurement);
  assert(!peer.server.control.open); /* Only one actual sample after reboot. */
  /* A rebooted sensor negotiates a clean sequence without relying on timeouts. */
  GreenhouseClient_Init(&client,0x1234);
  for (unsigned i=0;i<8;++i) round_trip(&peer,&client,18000+i*100,0,0);
  assert(client.ready && !peer.server.have_measurement);
  GreenhouseClient_Measurement(&client,250,300,1);
  for (unsigned i=0;i<5;++i) round_trip(&peer,&client,20000+i*100,0,0);
  assert(!client.pending && client.remote.sequence==1);
  assert(!(client.remote.flags & SPI_STATUS_OPEN) && client.remote.pulse_us==1000);
  puts("Greenhouse SPI: both endpoints, delayed ACK, CRC, retries, replay, reboot, timeout, wrap and all humidity codes PASS");
  return 0;
}
