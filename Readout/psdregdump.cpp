 /** 
#******************************************************************************
#
# Via Vetraia, 11 - 55049 - Viareggio ITALY
# +390594388398 - www.caen.it
#
#***************************************************************************//**
# 

##
# @file psdregdump
# @brief dump DPP PSD registers from a digitizer board.
# @author Ron Fox (rfoxkendo@gmail.com)

*/
#include <iostream>
#include <CAENDigitizer.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
CAEN_DGTZ_ConnectionType linkType;
int                      linkNum;
int                      node;
uint32_t                 base(0);

// These are the per channel base addresses of registers to dump.
//    Full addresses are: 0xynyy where n is the channel number.

struct Register {
  std::string name;
  uint32_t    base;
  bool        solitary;
};

#ifdef HARD_CODED
std::vector<Register> regs = {                      // Ends in null name pointer
  {"Board config,    ", 0x8000, true},
  {"buffer org.      ", 0x800c, true},
  {"Acq MOde         ", 0x8100, true},
  {"gbl trgmask.     ", 0x810c, true},
  {"TRG-OUT GPO mask ", 0x8100, true},
  {"Front Panel ctl  ", 0x811c, true},
  {"Ch enables       ", 0x8120, true},
  {"Start/Stop delay ", 0x8170, true},
  {"DisableExtTrg",0x817c,true},
  {"Record length    ", 0x1020, false},
  {"Dynamic Range    ", 0x1028, false},
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
  {"Stop bits       ", 0x1040, false}
};
#else
std::vector<Register> regs;          // Get filled in from file to describe what's dumped.
#endif

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
    std::cerr << "     Note the file registers.txt will be read in in the cwd to\n";
    std::cerr << "     decide which registers to dump.\n";
    
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
dumpRegisters(int handle, const std::vector<Register>& regs)
{
    CAEN_DGTZ_ErrorCode status;
    for (int i =0; i < regs.size(); i++) {
      uint32_t rbase = regs[i].base;
      uint32_t value;
      std::cout << regs[i].name << " " << std::hex << rbase << std::dec << std::endl;
      int n = 16;
      if (regs[i].solitary) n = 1;
      for (int c = 0; c < n; c++) {
          uint32_t a =  rbase | (c << 8); // Address to dump.
          status = CAEN_DGTZ_ReadRegister(handle, a, &value);
          checkStatus("Reading register", status, a);
          std::cout << "  Ch " << c << " : " << std::hex << value << std::dec << std::endl;
      }
    }
}

/**
 * readRegisterDefs
 *    Read the register definitions from a text file.
 *    The file defines one register/register set per line.
 *    Each line has three fields:
 *    *  Register name (must be quoted if there are space).
 *    *  register base address.
 *    *  Solitary flag.  If true, only a single register is set, otherwise,
 *       16 registers, one for each channel are read.
 * The file read is registers.txt in the current working directory
*/
static void
readRegisterDefs()
{
  std::ifstream in("./registers.txt");
  in >> std::hex;
  while (!in.eof()) {
    std::string name;
    uint32_t    base;
    bool        solitary;
    
    in >> name >> base >> solitary;
    if (name != "") {
      Register reg = {name, base, solitary};
      regs.push_back(reg);
    }
  }
 
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
#ifndef HARD_CODED
    readRegisterDefs();
#endif
    processArgs(argc, argv);       // Fails if arg count wrong.
    
    status = CAEN_DGTZ_OpenDigitizer(linkType, linkNum, node, base, &handle);
    checkStatus("Open digitizer", status, 0);
    dumpRegisters(handle, regs);
    status = CAEN_DGTZ_CloseDigitizer(handle);
    checkStatus("Close Digitizer", status, 0);
    
    std::exit(EXIT_SUCCESS);
}
