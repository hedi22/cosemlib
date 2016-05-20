#include "hdlc.h"
#include "hdlc_config.h"


/*
 *
 * GreenBook Table 8 – Control field bit assignments of command and response frames
 *

        Command        Response         MSB           LSB
        I               I               R R R P/F S S S 0
        RR              RR              R R R P/F 0 0 0 1
        RNR             RNR             R R R P/F 0 1 0 1
        SNRM                            1 0 0  P  0 0 1 1
        DISC                            0 1 0  P  0 0 1 1
                        UA              0 1 1  F  0 0 1 1
                        DM              0 0 0  F  1 1 1 1
                        FRMR            1 0 0  F  0 1 1 1
        UI              UI              0 0 0 P/F 0 0 1 1
*/

#define CTRL_MASK_IFRAME    0x01U



/*
 * FCS lookup table as calculated by the table generator
 * comes from Amber documentation (copy)
 */
static const uint16_t fcstab[256] = { 0x0000U, 0x1189U, 0x2312U, 0x329bU, 0x4624U, 0x57adU, 0x6536U, 0x74bfU, 0x8c48U, 0x9dc1U,
                                           0xaf5aU, 0xbed3U, 0xca6cU, 0xdbe5U, 0xe97eU, 0xf8f7U, 0x1081U, 0x0108U, 0x3393U, 0x221aU,
                                           0x56a5U, 0x472cU, 0x75b7U, 0x643eU, 0x9cc9U, 0x8d40U, 0xbfdbU, 0xae52U, 0xdaedU, 0xcb64U,
                                           0xf9ffU, 0xe876U, 0x2102U, 0x308bU, 0x0210U, 0x1399U, 0x6726U, 0x76afU, 0x4434U, 0x55bdU,
                                           0xad4aU, 0xbcc3U, 0x8e58U, 0x9fd1U, 0xeb6eU, 0xfae7U, 0xc87cU, 0xd9f5U, 0x3183U, 0x200aU,
                                           0x1291U, 0x0318U, 0x77a7U, 0x662eU, 0x54b5U, 0x453cU, 0xbdcbU, 0xac42U, 0x9ed9U, 0x8f50U,
                                           0xfbefU, 0xea66U, 0xd8fdU, 0xc974U, 0x4204U, 0x538dU, 0x6116U, 0x709fU, 0x0420U, 0x15a9U,
                                           0x2732U, 0x36bbU, 0xce4cU, 0xdfc5U, 0xed5eU, 0xfcd7U, 0x8868U, 0x99e1U, 0xab7aU, 0xbaf3U,
                                           0x5285U, 0x430cU, 0x7197U, 0x601eU, 0x14a1U, 0x0528U, 0x37b3U, 0x263aU, 0xdecdU, 0xcf44U,
                                           0xfddfU, 0xec56U, 0x98e9U, 0x8960U, 0xbbfbU, 0xaa72U, 0x6306U, 0x728fU, 0x4014U, 0x519dU,
                                           0x2522U, 0x34abU, 0x0630U, 0x17b9U, 0xef4eU, 0xfec7U, 0xcc5cU, 0xddd5U, 0xa96aU, 0xb8e3U,
                                           0x8a78U, 0x9bf1U, 0x7387U, 0x620eU, 0x5095U, 0x411cU, 0x35a3U, 0x242aU, 0x16b1U, 0x0738U,
                                           0xffcfU, 0xee46U, 0xdcddU, 0xcd54U, 0xb9ebU, 0xa862U, 0x9af9U, 0x8b70U, 0x8408U, 0x9581U,
                                           0xa71aU, 0xb693U, 0xc22cU, 0xd3a5U, 0xe13eU, 0xf0b7U, 0x0840U, 0x19c9U, 0x2b52U, 0x3adbU,
                                           0x4e64U, 0x5fedU, 0x6d76U, 0x7cffU, 0x9489U, 0x8500U, 0xb79bU, 0xa612U, 0xd2adU, 0xc324U,
                                           0xf1bfU, 0xe036U, 0x18c1U, 0x0948U, 0x3bd3U, 0x2a5aU, 0x5ee5U, 0x4f6cU, 0x7df7U, 0x6c7eU,
                                           0xa50aU, 0xb483U, 0x8618U, 0x9791U, 0xe32eU, 0xf2a7U, 0xc03cU, 0xd1b5U, 0x2942U, 0x38cbU,
                                           0x0a50U, 0x1bd9U, 0x6f66U, 0x7eefU, 0x4c74U, 0x5dfdU, 0xb58bU, 0xa402U, 0x9699U, 0x8710U,
                                           0xf3afU, 0xe226U, 0xd0bdU, 0xc134U, 0x39c3U, 0x284aU, 0x1ad1U, 0x0b58U, 0x7fe7U, 0x6e6eU,
                                           0x5cf5U, 0x4d7cU, 0xc60cU, 0xd785U, 0xe51eU, 0xf497U, 0x8028U, 0x91a1U, 0xa33aU, 0xb2b3U,
                                           0x4a44U, 0x5bcdU, 0x6956U, 0x78dfU, 0x0c60U, 0x1de9U, 0x2f72U, 0x3efbU, 0xd68dU, 0xc704U,
                                           0xf59fU, 0xe416U, 0x90a9U, 0x8120U, 0xb3bbU, 0xa232U, 0x5ac5U, 0x4b4cU, 0x79d7U, 0x685eU,
                                           0x1ce1U, 0x0d68U, 0x3ff3U, 0x2e7aU, 0xe70eU, 0xf687U, 0xc41cU, 0xd595U, 0xa12aU, 0xb0a3U,
                                           0x8238U, 0x93b1U, 0x6b46U, 0x7acfU, 0x4854U, 0x59ddU, 0x2d62U, 0x3cebU, 0x0e70U, 0x1ff9U,
                                           0xf78fU, 0xe606U, 0xd49dU, 0xc514U, 0xb1abU, 0xa022U, 0x92b9U, 0x8330U, 0x7bc7U, 0x6a4eU,
                                           0x58d5U, 0x495cU, 0x3de3U, 0x2c6aU, 0x1ef1U, 0x0f78U };


/**
 * @brief Checksum computing methods
 * @remark This function comes from Amber documentation (copy)
 */
static uint16_t hdlc_check_sequence(const uint8_t *data, uint32_t length)
{
    register uint16_t fcs = 0xFFFFU;
    register const uint8_t *dataPtr = data;

    while (length > 0U)
    {
        length--;
        fcs = (fcs >> 8U) ^ fcstab[(fcs ^ *dataPtr) & 0xffU];
        dataPtr++;
    }

    fcs ^= 0xffffU;
    return (fcs);
}

void hdlc_init(hdlc_channel *chan)
{

}
