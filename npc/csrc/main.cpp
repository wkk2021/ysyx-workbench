#include <stdio.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vswitch.h"
#include "Vswitch___024root.h"

#define MAX_SIM_TIME 20
uint64_t sim_time = 0;

int main() {
  Vswitch *dut = new Vswitch;
  Verilated::traceEverOn(true);
  VerilatedVcdC *m_trace = new VerilatedVcdC;
  dut->trace(m_trace, 5);
  m_trace->open("waveform.vcd");

  while (sim_time < MAX_SIM_TIME) {
      int a = rand() & 1;
      int b = rand() & 1;
      dut->a = a;
      dut->b = b;
      dut->eval();
      printf("a = %d, b = %d, f = %d\n", a, b, dut->f);
      assert(dut->f == (a ^ b));
      m_trace->dump(sim_time);
      sim_time++;
  }

  m_trace->close();
  delete dut;
  exit(EXIT_SUCCESS);
  }
