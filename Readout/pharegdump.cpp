 /** 
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file regdump
# @brief dump DPP PHA registers from a digitizer board.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include <iostream>
#include <CAENDigitizer.h>
#include <cstdlib>


CAEN_DGTZ_ConnectionType linkType;
int                      linkNum;
int                      node;
uint32_t                 base(0);

// These are the per channel base addresses of registers to dump.
//    Full addresses are: 0xynyy where n is the channel number.

struct Register {
  const char* name;
  uint32_t    base;
  bool        solitary;
};

Register regs[] = {                      // Ends in null name pointer
  {"Board config,    ", 0x8000, true},
  {"Record length    ", 0x1020, false},
  {"Dynamic Range    ", 0x1028, false},
  {"PreTrigger      ", 0x1038, false},
  {"RC-CR2 smoothing ", 0x1054, false},
  {"Input Rise Time ", 0x1058, false},
  {"Trap Rise Time  ", 0x105c, false},
  {"Trap Flat Top   ", 0x1060, false},
  {"Peaking time    ", 0x1064, false},
  {"Decay Time      ", 0x1068, false},
  {"Trigger thresh  ", 0x106c, false},
  {"Rise T Val Win  ", 0x1070, false},
  {"Trigger holdoff ", 0x1074, false},
  {"Peak hold off   ", 0x1078, false},
  {"Baseline hld off", 0x107c, false},
  {"DPP Control     ", 0x1080, false},
  {"DC Offset       ", 0x1098, false},
  {"DPP Control 2   ", 0x10a0, false},
  {"Fine gain       ", 0x104c, false},
  {"Pre Trigger     ", 0x1038, false},
  {"Stop bits       ", 0x1040, false},
  {0, 0, false}
};

/**
 * Usage
 *    output program usage to cerr and exit.
 */
static
void Usage()
{
    std::cerr << "Usage\n";
    std::cerr << "   regdump linktype linknum node [base]\n";
    std::cerr << "     linktype: 0 USB, 1 Optical\n";
    std::cerr << "     base is needed if the link is to a VME bus bridge and is\n";
    std::cerr << "     the module's base address\n";
    
    std::exit(EXIT_FAILURE);
}


/**
 * Turn the command line argts into the global connection params
 *
 * @param argc -number of command args.
 * @param argv - Parameters.
 * 
 */
static void processArgs(int argc, char** argv)
{
    if (argc < 4) {
        Usage();                                 // Exits.
    }
    linkType = static_cast<CAEN_DGTZ_ConnectionType>(strtoul(argv[1], NULL, 0 ));
    linkNum  = strtoul(argv[2], NULL, 0);
    node     = strtoul(argv[3], NULL, 0);
    if (argc > 4) {
        base = strtoul(argv[4], NULL, 0);
    }
}

/**
 * Check status;
 *    @param txt    - Text for the error message.
 *    @param status - return value from a digitizer function.
 *    @param value  - Additional value (output in hex).
 *    @note exits on error.
 */
static void
checkStatus(const char* txt, CAEN_DGTZ_ErrorCode status, int value)
{
    if (status != CAEN_DGTZ_Success) {
        std::cerr << "Failure: " << txt << " " << status
            << " : " << std::hex << value << std:: dec << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

/**
 * dumpRegisters
 *    Table driven register dump.
 *
 * @param handle - handle open on the digitizer.
 * @param regs   - Table of name/address pairs to dump.
 */
static void
dumpRegisters(int handle, Register* pRegs)
{
    CAEN_DGTZ_ErrorCode status;
    while (pRegs->name) {
        uint32_t rbase = pRegs->base;
        uint32_t value;
        std::cout << pRegs->name << " " << std::hex << rbase << std::dec << std::endl;
	int n = 16;
	if (pRegs->solitary) n = 1;
        for (int c = 0; c < n; c++) {
            uint32_t a =  rbase | (c << 8); // Address to dump.
            status = CAEN_DGTZ_ReadRegister(handle, a, &value);
            checkStatus("Reading register", status, a);
            std::cout << "  Addr "<< std::hex  << a  << " : " << value << std::endl;
        }
        pRegs++;
    }


   //Read global trigger mask
   uint32_t value;
   uint32_t value2 = 0x0001;
   uint32_t a = 0x810c;
   //status = CAEN_DGTZ_WriteRegister(handle, a, value2);
   //checkStatus("Reading register", status, a);
   int regarray[61] = {0x8020,0x1088,0x108c,0x10A8,0x8000,0x800c,0x8100,0x8104,0x810c,0x8110,0x8114, 0x8118,0x811c,0x8120,0x8124,0x812c,0x8138,0x8140,0x8144,0x814c,0x8168,0x816c,0x8170,0x8178,0x817c,0x8180,0x81a0,0x81b4,0xef00,0xef04,0xef08,0xef0c,0xef10,0xef14,0xef18,0xef1c,0xef20,0xf000,0xf004,0xf008,0xf00c,0xf010,0xf014,0xf018,0xf01c,0xf020,0xf024,0xf028,0xf02c,0xf030,0xf034,0xf038,0xf03c,0xf040,0xf044,0xf048,0xf04c,0xf050,0xf080,0xf084,0xf088};

   for(int ctr = 0; ctr<60; ctr++)
   {
    a = regarray[ctr];
    std::cout<< '\n' << a;
    //return ;
    value = 0;
    status = CAEN_DGTZ_ReadRegister(handle, a, &value);
    checkStatus("Reading register", status, a);
    std::cout << "  Addr "<< std::hex  << a  << " : " << value << std::endl;
   }//*/

   /*a = 0x812c;
   status = CAEN_DGTZ_ReadRegister(handle, a, &value);
   checkStatus("Reading register", status, a);
   std::cout << "  Addr "<< std::hex  << a  << " : " << value << std::endl;*/


}

/**
 * main
 *    Entry point.
 *       regdump type link node base
 */
int
main(int argc, char** argv)
{
    CAEN_DGTZ_ErrorCode status;
    int                 handle;
    
    processArgs(argc, argv);       // Fails if arg count wrong.
    
    status = CAEN_DGTZ_OpenDigitizer(linkType, linkNum, node, base, &handle);
    checkStatus("Open digitizer", status, 0);
    dumpRegisters(handle, regs);
    status = CAEN_DGTZ_CloseDigitizer(handle);
    checkStatus("Close Digitizer", status, 0);
    
    std::exit(EXIT_SUCCESS);
}
