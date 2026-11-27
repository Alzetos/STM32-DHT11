#include "main.h"
#include "greenhouse_motor.h"
#include "stepper_control.h"
#include "monitor_config.h"
#include <assert.h>
#include <stdio.h>
GPIO_TypeDef mock_port;
static uint32_t odr = (1U << 10) | (1U << 11) | (1U << 12);
static const uint32_t spi_pins = (1U << 10) | (1U << 11) | (1U << 12);
static const uint32_t motor_pins = (1U << 6) | (1U << 7) | (1U << 8) | (1U << 9);
static void tick(unsigned count) {
  while (count--) {
    StepperControl_Tick1ms();
    uint32_t write = mock_port.BSRR;
    assert(((write | (write >> 16)) & 0xFFFFU & ~motor_pins) == 0);
    odr = (odr & ~(write >> 16)) | (write & 0xFFFFU);
    assert((odr & spi_pins) == spi_pins);
  }
}
static void receive(GreenhouseServer *s, uint8_t type, uint16_t seq, unsigned h, int valid, uint32_t now) {
  LinkMessage m = {0}; uint8_t bytes[LINK_PACKET_SIZE];
  m.type = type; m.sequence = seq;
  if (type == LINK_TYPE_MEASUREMENT) { m.temperature10 = 250; m.humidity10 = (uint16_t)h; m.flags = valid ? LINK_DATA_VALID : 0; }
  LinkProtocol_Encode(bytes, &m); assert(LinkProtocol_Decode(&m,bytes));
  GreenhouseServer_Receive(s,&m,now);
  GreenhouseMotor_Update(s,now);
}
static LinkMessage status(GreenhouseServer *s, uint32_t now) {
  LinkMessage m; uint8_t bytes[LINK_PACKET_SIZE];
  GreenhouseServer_Status(s,now,StepperControl_GetPositionSteps(),StepperControl_IsMoving(),&m);
  LinkProtocol_Encode(bytes,&m); assert(LinkProtocol_Decode(&m,bytes));
  return m;
}
int main(void) {
  GreenhouseServer s; GreenhouseServer_Init(&s); GreenhouseMotor_Init();
  GreenhouseMotor_Update(&s,0); tick(100);
  assert(!StepperControl_IsMoving() && StepperControl_GetPositionSteps()==0);
  receive(&s,LINK_TYPE_HELLO,10,0,0,0);
  receive(&s,LINK_TYPE_MEASUREMENT,1,500,1,10); assert(!s.control.open);
  receive(&s,LINK_TYPE_MEASUREMENT,2,500,1,2210); assert(s.control.open);
  assert(StepperControl_GetTargetSteps()==GH_STEPPER_OPEN_STEPS);
  LinkMessage m = status(&s,2210);
  assert((m.flags & LINK_STATUS_MOVING) && !(m.flags & LINK_STATUS_OPEN));
  assert((m.detail & LINK_DETAIL_TARGET_OPEN) && !(m.detail & LINK_DETAIL_AT_CLOSED));
  tick(20); uint16_t middle = StepperControl_GetPositionSteps();
  assert(middle>0 && middle<GH_STEPPER_OPEN_STEPS);
  receive(&s,LINK_TYPE_MEASUREMENT,3,0,0,2300); tick(100);
  assert(StepperControl_GetPositionSteps()==middle && !StepperControl_IsMoving());
  assert(odr & motor_pins); /* Fault stop holds coils even if normal hold-at-target=0. */
  m=status(&s,2300);
  assert(!(m.flags & (LINK_STATUS_MOVING | LINK_STATUS_OPEN)) && !(m.detail & LINK_DETAIL_AT_CLOSED));
  receive(&s,LINK_TYPE_MEASUREMENT,4,500,1,4500);
  tick((GH_STEPPER_OPEN_STEPS+2U)*STEPPER_STEP_INTERVAL_MS);
  assert(StepperControl_GetPositionSteps()==GH_STEPPER_OPEN_STEPS && !StepperControl_IsMoving());
  m=status(&s,8000); assert((m.flags & LINK_STATUS_OPEN) && m.position_steps==GH_STEPPER_OPEN_STEPS);
  GreenhouseMotor_Update(&s,14500); tick(100);
  m=status(&s,14500); assert((m.flags & LINK_STATUS_STALE) && !StepperControl_IsMoving());
  receive(&s,LINK_TYPE_MEASUREMENT,5,300,1,15000); assert(s.control.open);
  receive(&s,LINK_TYPE_MEASUREMENT,6,300,1,17200); assert(!s.control.open);
  tick(20); middle=StepperControl_GetPositionSteps();
  assert(middle<GH_STEPPER_OPEN_STEPS && middle>0);
  m=status(&s,17200); assert((m.flags & LINK_STATUS_MOVING) && !(m.detail & LINK_DETAIL_TARGET_OPEN));
  receive(&s,LINK_TYPE_HELLO,11,0,0,17300); tick(100);
  assert(StepperControl_GetPositionSteps()==middle && !StepperControl_IsMoving());
  receive(&s,LINK_TYPE_MEASUREMENT,1,300,1,18000);
  tick((GH_STEPPER_OPEN_STEPS+2U)*STEPPER_STEP_INTERVAL_MS);
  assert(StepperControl_GetPositionSteps()==0);
  m=status(&s,22000); assert((m.detail & LINK_DETAIL_AT_CLOSED) && !(m.flags & LINK_STATUS_OPEN));
  assert(!StepperControl_SetTargetSteps((uint16_t)(STEPPER_STEPS_PER_REV+1U)));
  tick(10); assert(!(odr & motor_pins));
  assert(StepperControl_SetTargetSteps(50)); tick(40); middle=StepperControl_GetPositionSteps();
  StepperControl_Stop(1); tick(100); assert(StepperControl_GetPositionSteps()==middle && (odr & motor_pins));
  StepperControl_Stop(0); tick(10); assert(!(odr & motor_pins));
  puts("PASS greenhouse stepper: actual motion, endpoints, OPENING/CLOSING/STOPPED, errors, resync and other GPIOC pins preserved");
}
