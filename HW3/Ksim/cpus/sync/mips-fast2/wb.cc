#include "wb.h"

Writeback::Writeback (Mipc *mc)
{
   _mc = mc;
}

Writeback::~Writeback (void) {}

void
Writeback::MainLoop (void)
{
   unsigned int ins;
   Bool writeReg;
   Bool writeFReg;
   Bool loWPort;
   Bool hiWPort;
   Bool isSyscall;
   Bool isIllegalOp;
   unsigned decodedDST;
   unsigned opResultLo, opResultHi;

   while (1) {
      AWAIT_P_PHI0;	// @posedge
      // Sample the important signals
      if (_mc->_memValid) {
         ins           = _mc->_pipeReg_mw[0] ;
         isSyscall     = _mc->_pipeReg_mw[1] ;
         isIllegalOp   = _mc->_pipeReg_mw[2]; 
         writeReg      = _mc->_pipeReg_mw[3] ;
         writeFReg     = _mc->_pipeReg_mw[4]; 
         loWPort       = _mc->_pipeReg_mw[5] ;
         hiWPort       = _mc->_pipeReg_mw[6] ;
         decodedDST    = _mc->_pipeReg_mw[7] ;
         opResultLo    = _mc->_pipeReg_mw[8] ;
         opResultHi    = _mc->_pipeReg_mw[9] ;

         // writeReg = _mc->_writeREG;
         // writeFReg = _mc->_writeFREG;
         // loWPort = _mc->_loWPort;
         // hiWPort = _mc->_hiWPort;
         // decodedDST = _mc->_decodedDST;
         // opResultLo = _mc->_opResultLo;
         // opResultHi = _mc->_opResultHi;
         // isSyscall = _mc->_isSyscall;
         // isIllegalOp = _mc->_isIllegalOp;
         // ins = _mc->_ins;
         
         AWAIT_P_PHI1;       // @negedge
         if (isSyscall) {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> SYSCALL! Trapping to emulation layer at PC %#x\n", SIM_TIME, _mc->_pc);
#endif      
            _mc->_opControl(_mc, ins);
            // _mc->_pc += 4;
         }
         else if (isIllegalOp) {
            printf("Illegal ins %#x at PC %#x. Terminating simulation!\n", ins, _mc->_pc);
#ifdef MIPC_DEBUG
            fclose(_mc->_debugLog);
#endif
            printf("Register state on termination:\n\n");
            _mc->dumpregs();
            exit(0);
         }
         else {
            if (writeReg) {
               _mc->_gpr[decodedDST] = opResultLo;
		_mc->popFromROB();
#ifdef MIPC_DEBUG
               fprintf(_mc->_debugLog, "<%llu> Writing to reg %u, value: %#x for ins: %#x\n", SIM_TIME, decodedDST, opResultLo, ins);
#endif
            }
            else if (writeFReg) {
               _mc->_fpr[(decodedDST)>>1].l[FP_TWIDDLE^((decodedDST)&1)] = opResultLo;
#ifdef MIPC_DEBUG
               fprintf(_mc->_debugLog, "<%llu> Writing to freg %u, value: %#x for ins %#x\n", SIM_TIME, decodedDST>>1, opResultLo, ins);
#endif
            }
            else if (loWPort || hiWPort) {
               if (loWPort) {
                  _mc->_lo = opResultLo;
#ifdef MIPC_DEBUG
                  fprintf(_mc->_debugLog, "<%llu> Writing to Lo, value: %#x for ins %#x\n", SIM_TIME, opResultLo, ins);
#endif
               }
               if (hiWPort) {
                  _mc->_hi = opResultHi;
#ifdef MIPC_DEBUG
                  fprintf(_mc->_debugLog, "<%llu> Writing to Hi, value: %#x for ins %#x\n", SIM_TIME, opResultHi, ins);
#endif
               }
            }
         }
         _mc->_gpr[0] = 0;
      //    _mc->_memValid = FALSE;
         // _mc->_insDone = TRUE;
      }
      else {
         PAUSE(1);
      }
   }
}
