/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <linux/ata.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/regulator/consumer.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <linux/pwm_backlight.h>
#include <linux/fec.h>
#include <linux/memblock.h>
#include <linux/gpio.h>
#include <linux/ion.h>
#include <linux/etherdevice.h>
#include <linux/regulator/anatop-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/spi/ads7846.h>

#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/mxc_dvfs.h>
#include <mach/memory.h>
#include <mach/iomux-mx6q.h>
#include <mach/imx-uart.h>
#include <mach/viv_gpu.h>
#include <mach/ahci_sata.h>
#include <mach/ipu-v3.h>
#include <mach/mxc_hdmi.h>
#include <mach/mxc_asrc.h>

#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>

#include "usb.h"
#include "devices-imx6q.h"
#include "crm_regs.h"
#include "cpu_op-mx6.h"

#define MX6Q_SBC9000_SD2_CD     IMX_GPIO_NR(1, 4)
#define MX6Q_SBC9000_ECSPI1_CS0	IMX_GPIO_NR(2, 30)
#define MX6Q_SBC9000_USB_OTG_PWR	IMX_GPIO_NR(3, 22)
#define MX6Q_SBC9000_CAP_TCH_INT1	IMX_GPIO_NR(1, 9)
#define MX6Q_SBC9000_USB_HUB_RESET	IMX_GPIO_NR(7, 13)
#define MX6Q_SBC9000_ONOFF_KEY	IMX_GPIO_NR(2, 3)
#define MX6Q_SBC9000_HOME_KEY		IMX_GPIO_NR(4, 11)
#define MX6Q_SBC9000_CSI0_RST		IMX_GPIO_NR(3, 19)
#define MX6Q_SBC9000_CSI0_PWN		IMX_GPIO_NR(3, 20)

#define MX6Q_SBC9000_SYS_LED        IMX_GPIO_NR(1, 30)
#define MX6Q_SBC9000_USER1_LED        IMX_GPIO_NR(3, 28)
#define MX6Q_SBC9000_USER2_LED        IMX_GPIO_NR(3, 29)

#define MX6Q_SBC9000_LED_PWN        IMX_GPIO_NR(7, 12)

#define MX6_ENET_IRQ		IMX_GPIO_NR(1, 28)
#define IOMUX_OBSRV_MUX1_OFFSET	0x3c
#define OBSRV_MUX1_MASK			0x3f
#define OBSRV_MUX1_ENET_IRQ		0x9

#define MX6Q_SBC9000_SD3_WP_PADCFG	(PAD_CTL_PKE | PAD_CTL_PUE |	\
		PAD_CTL_PUS_22K_UP | PAD_CTL_SPEED_MED |	\
		PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define MX6Q_SBC9000_PCIE_WAKE_B      IMX_GPIO_NR(1, 27)
#define MX6Q_SBC9000_PCIE_W_DIS       IMX_GPIO_NR(7, 6)
#define MX6Q_SBC9000_PCIE_PERST       IMX_GPIO_NR(7, 1)

#define MX6Q_SBC9000_RES_TCH_INT      IMX_GPIO_NR(6, 11)

void __init early_console_setup(unsigned long base, struct clk *clk);
static struct clk *sata_clk;

extern char *gp_reg_id;
extern char *soc_reg_id;
extern char *pu_reg_id;
extern bool enet_to_gpio_6;
static int caam_enabled;

extern struct regulator *(*get_cpu_regulator)(void);
extern void (*put_cpu_regulator)(void);

static iomux_v3_cfg_t mx6q_sbc9000_pads[] = {
	/* AUDMUX */
    MX6Q_PAD_CSI0_DAT4__AUDMUX_AUD3_TXC,
    MX6Q_PAD_CSI0_DAT5__AUDMUX_AUD3_TXD,
    MX6Q_PAD_CSI0_DAT6__AUDMUX_AUD3_TXFS,
    MX6Q_PAD_CSI0_DAT7__AUDMUX_AUD3_RXD,

	/* LEDS*/
	MX6Q_PAD_EIM_D28__GPIO_3_28,
	MX6Q_PAD_EIM_D29__GPIO_3_29,
	MX6Q_PAD_ENET_TXD0__GPIO_1_30,
	
	/* BUTTON */
	MX6Q_PAD_KEY_ROW2__GPIO_4_11,

	/* CAMERA */
	MX6Q_PAD_EIM_D19__GPIO_3_19,    /* CAM_RST */
	MX6Q_PAD_EIM_D20__GPIO_3_20,    /* CMA_EN*/

	/* ADS7846 TOUCH INT */
	MX6Q_PAD_NANDF_CS0__GPIO_6_11,

	/* CAPACITIVE TOUCH INT */
	MX6Q_PAD_NANDF_CS1__GPIO_6_14,
	
	/* CAPACITIVE RESET */
	MX6Q_PAD_ENET_TXD1__GPIO_1_29,

	/* CAN1  */
	MX6Q_PAD_GPIO_7__CAN1_TXCAN,
	MX6Q_PAD_GPIO_8__CAN1_RXCAN,

	MX6Q_PAD_KEY_COL4__CAN2_TXCAN,
    MX6Q_PAD_KEY_ROW4__CAN2_RXCAN,   

	/* CCM  */
	MX6Q_PAD_GPIO_0__CCM_CLKO,		/* SGTL5000 sys_mclk */

	/* PCIE_WAKEn */
	MX6Q_PAD_GPIO_3__GPIO_1_3,	
	MX6Q_PAD_ENET_RXD0__GPIO_1_27,  /* PCIE_WAKE_B */
	MX6Q_PAD_SD3_DAT2__GPIO_7_6,    /* PCIE_W_DIS */
	MX6Q_PAD_SD3_DAT4__GPIO_7_1,    /* PCIE_RST */

	/* ECSPI1 */
	MX6Q_PAD_EIM_D17__ECSPI1_MISO,
	MX6Q_PAD_EIM_D18__ECSPI1_MOSI,
	MX6Q_PAD_EIM_D16__ECSPI1_SCLK,
	MX6Q_PAD_EIM_EB2__GPIO_2_30,    /*CS0*/
    MX6Q_PAD_KEY_COL2__GPIO_4_10,   /*CS1*/

	/* ENET */
	MX6Q_PAD_ENET_MDIO__ENET_MDIO,
	MX6Q_PAD_ENET_MDC__ENET_MDC,
	MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC,
	MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0,
	MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1,
	MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2,
	MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3,
	MX6Q_PAD_RGMII_TX_CTL__ENET_RGMII_TX_CTL,
	MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK,
	MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC,
	MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0,
	MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1,
	MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2,
	MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3,
	MX6Q_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,
	MX6Q_PAD_ENET_TX_EN__GPIO_1_28,		/* Micrel RGMII Phy Interrupt */
	MX6Q_PAD_EIM_D31__GPIO_3_31,		/* RGMII reset */

	/* GPIO2 */
	MX6Q_PAD_EIM_A22__GPIO_2_16,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A21__GPIO_2_17,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A20__GPIO_2_18,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A19__GPIO_2_19,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A18__GPIO_2_20,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A17__GPIO_2_21,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A16__GPIO_2_22,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_RW__GPIO_2_26,		/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_LBA__GPIO_2_27,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_EB0__GPIO_2_28,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_EB1__GPIO_2_29,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_EB3__GPIO_2_31,	/* J12 - Boot Mode Select */

	/* GPIO3 */
	MX6Q_PAD_EIM_DA0__GPIO_3_0,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA1__GPIO_3_1,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA2__GPIO_3_2,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA3__GPIO_3_3,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA4__GPIO_3_4,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA5__GPIO_3_5,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA6__GPIO_3_6,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA7__GPIO_3_7,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA8__GPIO_3_8,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA9__GPIO_3_9,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA10__GPIO_3_10,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA11__GPIO_3_11,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA12__GPIO_3_12,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA13__GPIO_3_13,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA14__GPIO_3_14,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_DA15__GPIO_3_15,	/* J12 - Boot Mode Select */

	/* GPIO4 */
	MX6Q_PAD_GPIO_19__GPIO_4_5,	/* J14 - Volume Down */

	/* GPIO5 */
	MX6Q_PAD_EIM_WAIT__GPIO_5_0,	/* J12 - Boot Mode Select */
	MX6Q_PAD_EIM_A24__GPIO_5_4,	/* J12 - Boot Mode Select */

	/* GPIO6 */
	MX6Q_PAD_EIM_A23__GPIO_6_6,	/* J12 - Boot Mode Select */

	/* GPIO7 */
	MX6Q_PAD_GPIO_17__GPIO_7_12,	/* LED_PWR_EN for LVDS*/
	MX6Q_PAD_GPIO_18__GPIO_7_13,	/* USB_RSTn */

	/* I2C1, SGTL5000 */
    MX6Q_PAD_CSI0_DAT8__I2C1_SDA,   /* GPIO5[26] */
	MX6Q_PAD_CSI0_DAT9__I2C1_SCL,   /* GPIO5[27] */

	/* I2C2 */
    MX6Q_PAD_KEY_COL3__I2C2_SCL,    /* GPIO4[12] */
    MX6Q_PAD_KEY_ROW3__I2C2_SDA,    /* GPIO4[13] */

	/* I2C3 */
	MX6Q_PAD_GPIO_5__I2C3_SCL,	/* GPIO1[5] */
	MX6Q_PAD_GPIO_6__I2C3_SDA,	/* GPIO1[6] */

	/* DISPLAY */
	MX6Q_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6Q_PAD_DI0_PIN15__IPU1_DI0_PIN15,		/* DE */
	MX6Q_PAD_DI0_PIN2__IPU1_DI0_PIN2,		/* HSync */
	MX6Q_PAD_DI0_PIN3__IPU1_DI0_PIN3,		/* VSync */
	MX6Q_PAD_DI0_PIN4__IPU1_DI0_PIN4,		/* Contrast */
	MX6Q_PAD_DISP0_DAT0__IPU1_DISP0_DAT_0,
	MX6Q_PAD_DISP0_DAT1__IPU1_DISP0_DAT_1,
	MX6Q_PAD_DISP0_DAT2__IPU1_DISP0_DAT_2,
	MX6Q_PAD_DISP0_DAT3__IPU1_DISP0_DAT_3,
	MX6Q_PAD_DISP0_DAT4__IPU1_DISP0_DAT_4,
	MX6Q_PAD_DISP0_DAT5__IPU1_DISP0_DAT_5,
	MX6Q_PAD_DISP0_DAT6__IPU1_DISP0_DAT_6,
	MX6Q_PAD_DISP0_DAT7__IPU1_DISP0_DAT_7,
	MX6Q_PAD_DISP0_DAT8__IPU1_DISP0_DAT_8,
	MX6Q_PAD_DISP0_DAT9__IPU1_DISP0_DAT_9,
	MX6Q_PAD_DISP0_DAT10__IPU1_DISP0_DAT_10,
	MX6Q_PAD_DISP0_DAT11__IPU1_DISP0_DAT_11,
	MX6Q_PAD_DISP0_DAT12__IPU1_DISP0_DAT_12,
	MX6Q_PAD_DISP0_DAT13__IPU1_DISP0_DAT_13,
	MX6Q_PAD_DISP0_DAT14__IPU1_DISP0_DAT_14,
	MX6Q_PAD_DISP0_DAT15__IPU1_DISP0_DAT_15,
	MX6Q_PAD_DISP0_DAT16__IPU1_DISP0_DAT_16,
	MX6Q_PAD_DISP0_DAT17__IPU1_DISP0_DAT_17,
	MX6Q_PAD_DISP0_DAT18__IPU1_DISP0_DAT_18,
	MX6Q_PAD_DISP0_DAT19__IPU1_DISP0_DAT_19,
	MX6Q_PAD_DISP0_DAT20__IPU1_DISP0_DAT_20,
	MX6Q_PAD_DISP0_DAT21__IPU1_DISP0_DAT_21,
	MX6Q_PAD_DISP0_DAT22__IPU1_DISP0_DAT_22,
	MX6Q_PAD_DISP0_DAT23__IPU1_DISP0_DAT_23,
	MX6Q_PAD_NANDF_D0__GPIO_2_0,		/* J6 - LVDS Display contrast */


	/* PWM1 */
	MX6Q_PAD_GPIO_9__PWM1_PWMO,         /* GPIO1[9] for LVDS backlight*/

	/* PWM2 */
	MX6Q_PAD_GPIO_1__PWM2_PWMO,         /* GPIO1[1] for LCD backlight*/

	/* PWM3 */

	/* PWM4 */

	/* UART1  */
	MX6Q_PAD_SD3_DAT7__UART1_TXD,
	MX6Q_PAD_SD3_DAT6__UART1_RXD,

	/* UART2 for debug */
	MX6Q_PAD_EIM_D26__UART2_TXD,
	MX6Q_PAD_EIM_D27__UART2_RXD,
	MX6Q_PAD_SD3_CMD__UART2_CTS,
	MX6Q_PAD_SD3_CLK__UART2_RTS,

	/* UART3 */
	MX6Q_PAD_EIM_D24__UART3_TXD,
	MX6Q_PAD_EIM_D25__UART3_RXD,
	MX6Q_PAD_EIM_D23__UART3_CTS,
	MX6Q_PAD_SD3_RST__UART3_RTS,

	/* UART5 */
	MX6Q_PAD_KEY_COL1__UART5_TXD,
	MX6Q_PAD_KEY_ROW1__UART5_RXD,
	

	/* USBOTG ID pin */
	MX6Q_PAD_ENET_RX_ER__ANATOP_USBOTG_ID,

	/* USB OC pin */
	MX6Q_PAD_EIM_D30__USBOH3_USBH1_OC,
	MX6Q_PAD_EIM_D21__USBOH3_USBOTG_OC,

	/* USB power pin */
	MX6Q_PAD_EIM_D22__GPIO_3_22,

	/* USDHC2 */
	MX6Q_PAD_SD2_CLK__USDHC2_CLK_50MHZ,
	MX6Q_PAD_SD2_CMD__USDHC2_CMD_50MHZ,
	MX6Q_PAD_SD2_DAT0__USDHC2_DAT0_50MHZ,
	MX6Q_PAD_SD2_DAT1__USDHC2_DAT1_50MHZ,
	MX6Q_PAD_SD2_DAT2__USDHC2_DAT2_50MHZ,
	MX6Q_PAD_SD2_DAT3__USDHC2_DAT3_50MHZ,
	MX6Q_PAD_GPIO_4__GPIO_1_4,            	/* SD2_CD */
	MX6Q_PAD_NANDF_D5__USDHC2_DAT5,
	MX6Q_PAD_NANDF_D6__USDHC2_DAT6,
	MX6Q_PAD_NANDF_D7__USDHC2_DAT7,

	/* USDHC3 */

	/* USDHC4 */
	MX6Q_PAD_SD4_CLK__USDHC4_CLK_50MHZ,
	MX6Q_PAD_SD4_CMD__USDHC4_CMD_50MHZ,
	MX6Q_PAD_SD4_DAT0__USDHC4_DAT0_50MHZ,
	MX6Q_PAD_SD4_DAT1__USDHC4_DAT1_50MHZ,
	MX6Q_PAD_SD4_DAT2__USDHC4_DAT2_50MHZ,
	MX6Q_PAD_SD4_DAT3__USDHC4_DAT3_50MHZ,
	MX6Q_PAD_SD4_DAT4__USDHC4_DAT4_50MHZ,
	MX6Q_PAD_SD4_DAT5__USDHC4_DAT5_50MHZ,
	MX6Q_PAD_SD4_DAT6__USDHC4_DAT6_50MHZ,
	MX6Q_PAD_SD4_DAT7__USDHC4_DAT7_50MHZ,
	MX6Q_PAD_NANDF_ALE__GPIO_6_8,		

};

static iomux_v3_cfg_t mx6q_sbc9000_csi0_sensor_pads[] = {
	/* IPU1 Camera */
	MX6Q_PAD_CSI0_DAT12__IPU1_CSI0_D_12,
	MX6Q_PAD_CSI0_DAT13__IPU1_CSI0_D_13,
	MX6Q_PAD_CSI0_DAT14__IPU1_CSI0_D_14,
	MX6Q_PAD_CSI0_DAT15__IPU1_CSI0_D_15,
	MX6Q_PAD_CSI0_DAT16__IPU1_CSI0_D_16,
	MX6Q_PAD_CSI0_DAT17__IPU1_CSI0_D_17,
	MX6Q_PAD_CSI0_DAT18__IPU1_CSI0_D_18,
	MX6Q_PAD_CSI0_DAT19__IPU1_CSI0_D_19,
	MX6Q_PAD_CSI0_MCLK__IPU1_CSI0_HSYNC,
	MX6Q_PAD_CSI0_PIXCLK__IPU1_CSI0_PIXCLK,
	MX6Q_PAD_CSI0_VSYNC__IPU1_CSI0_VSYNC,
};

static iomux_v3_cfg_t mx6q_sbc9000_hdmi_ddc_pads[] = {
	MX6Q_PAD_KEY_COL3__HDMI_TX_DDC_SCL, /* HDMI DDC SCL */
	MX6Q_PAD_KEY_ROW3__HDMI_TX_DDC_SDA, /* HDMI DDC SDA */
};

static iomux_v3_cfg_t mx6q_sbc9000_i2c2_pads[] = {
	MX6Q_PAD_KEY_COL3__I2C2_SCL,	/* I2C2 SCL */
	MX6Q_PAD_KEY_ROW3__I2C2_SDA,	/* I2C2 SDA */
};

#define MX6Q_USDHC_PAD_SETTING(id, speed)	\
mx6q_sd##id##_##speed##mhz[] = {		\
	MX6Q_PAD_SD##id##_CLK__USDHC##id##_CLK_##speed##MHZ,	\
	MX6Q_PAD_SD##id##_CMD__USDHC##id##_CMD_##speed##MHZ,	\
	MX6Q_PAD_SD##id##_DAT0__USDHC##id##_DAT0_##speed##MHZ,	\
	MX6Q_PAD_SD##id##_DAT1__USDHC##id##_DAT1_##speed##MHZ,	\
	MX6Q_PAD_SD##id##_DAT2__USDHC##id##_DAT2_##speed##MHZ,	\
	MX6Q_PAD_SD##id##_DAT3__USDHC##id##_DAT3_##speed##MHZ,	\
}

static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(2, 50);
static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(2, 100);
static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(2, 200);
static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(4, 50);
static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(4, 100);
static iomux_v3_cfg_t MX6Q_USDHC_PAD_SETTING(4, 200);

enum sd_pad_mode {
	SD_PAD_MODE_LOW_SPEED,
	SD_PAD_MODE_MED_SPEED,
	SD_PAD_MODE_HIGH_SPEED,
};

static int plt_sd_pad_change(unsigned int index, int clock)
{
	/* LOW speed is the default state of SD pads */
	static enum sd_pad_mode pad_mode = SD_PAD_MODE_LOW_SPEED;

	iomux_v3_cfg_t *sd_pads_200mhz = NULL;
	iomux_v3_cfg_t *sd_pads_100mhz = NULL;
	iomux_v3_cfg_t *sd_pads_50mhz = NULL;

	u32 sd_pads_200mhz_cnt;
	u32 sd_pads_100mhz_cnt;
	u32 sd_pads_50mhz_cnt;

	switch (index) {
	case 1:
		sd_pads_200mhz = mx6q_sd2_200mhz;
		sd_pads_100mhz = mx6q_sd2_100mhz;
		sd_pads_50mhz = mx6q_sd2_50mhz;

		sd_pads_200mhz_cnt = ARRAY_SIZE(mx6q_sd2_200mhz);
		sd_pads_100mhz_cnt = ARRAY_SIZE(mx6q_sd2_100mhz);
		sd_pads_50mhz_cnt = ARRAY_SIZE(mx6q_sd2_50mhz);
		break;
	case 3:
		sd_pads_200mhz = mx6q_sd4_200mhz;
		sd_pads_100mhz = mx6q_sd4_100mhz;
		sd_pads_50mhz = mx6q_sd4_50mhz;

		sd_pads_200mhz_cnt = ARRAY_SIZE(mx6q_sd4_200mhz);
		sd_pads_100mhz_cnt = ARRAY_SIZE(mx6q_sd4_100mhz);
		sd_pads_50mhz_cnt = ARRAY_SIZE(mx6q_sd4_50mhz);
		break;
	default:
		printk(KERN_ERR "no such SD host controller index %d\n", index);
		return -EINVAL;
	}

	if (clock > 100000000) {
		if (pad_mode == SD_PAD_MODE_HIGH_SPEED)
			return 0;
		BUG_ON(!sd_pads_200mhz);
		pad_mode = SD_PAD_MODE_HIGH_SPEED;
		return mxc_iomux_v3_setup_multiple_pads(sd_pads_200mhz,
							sd_pads_200mhz_cnt);
	} else if (clock > 52000000) {
		if (pad_mode == SD_PAD_MODE_MED_SPEED)
			return 0;
		BUG_ON(!sd_pads_100mhz);
		pad_mode = SD_PAD_MODE_MED_SPEED;
		return mxc_iomux_v3_setup_multiple_pads(sd_pads_100mhz,
							sd_pads_100mhz_cnt);
	} else {
		if (pad_mode == SD_PAD_MODE_LOW_SPEED)
			return 0;
		BUG_ON(!sd_pads_50mhz);
		pad_mode = SD_PAD_MODE_LOW_SPEED;
		return mxc_iomux_v3_setup_multiple_pads(sd_pads_50mhz,
							sd_pads_50mhz_cnt);
	}
}

static const struct esdhc_platform_data mx6q_sbc9000_sd2_data __initconst = {
	.cd_gpio = MX6Q_SBC9000_SD2_CD,
	.wp_gpio = -EINVAL,
	.keep_power_at_suspend = 1,
	.platform_pad_change = plt_sd_pad_change,
};

static const struct esdhc_platform_data mx6q_sbc9000_sd4_data __initconst = {
	.cd_gpio = -EINVAL,
	.wp_gpio = -EINVAL,
	.keep_power_at_suspend = 1,
	.platform_pad_change = plt_sd_pad_change,
};

static const struct anatop_thermal_platform_data
	mx6q_sbc9000_anatop_thermal_data __initconst = {
		.name = "anatop_thermal",
};

static inline void mx6q_sbc9000_init_uart(void)
{
	imx6q_add_imx_uart(0, NULL);
	imx6q_add_imx_uart(1, NULL);
	imx6q_add_imx_uart(2, NULL);
	imx6q_add_imx_uart(4, NULL);

}

static int mx6q_sbc9000_fec_phy_init(struct phy_device *phydev)
{
	unsigned short val;

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x8016);
	phy_write(phydev, 0xd, 0x4007);
	val = phy_read(phydev, 0xe);

	val &= 0xffe3;
	val |= 0x18;
	phy_write(phydev, 0xe, val);

	/* Introduce tx clock delay */
	phy_write(phydev, 0x1d, 0x5);
	val = phy_read(phydev, 0x1e);
	val |= 0x0100;
	phy_write(phydev, 0x1e, val);

	/* rgmii gtx clock delay */
	phy_write(phydev, 0x1d, 0xb);
	val = phy_read(phydev, 0x1e);
	val &= ~0x60;
	val |= 0x20;
	phy_write(phydev, 0x1e, val);

	/*check phy power*/
	val = phy_read(phydev, 0x0);

	if (val & BMCR_PDOWN)
		phy_write(phydev, 0x0, (val & ~BMCR_PDOWN));

	return 0;
}

static struct fec_platform_data fec_data __initdata = {
	.init = mx6q_sbc9000_fec_phy_init,
	.phy = PHY_INTERFACE_MODE_RGMII,
//	.gpio_irq = MX6_ENET_IRQ,
};

static int mx6q_sbc9000_spi_cs[] = {
	MX6Q_SBC9000_ECSPI1_CS0,
};

static const struct spi_imx_master mx6q_sbc9000_spi_data __initconst = {
	.chipselect     = mx6q_sbc9000_spi_cs,
	.num_chipselect = ARRAY_SIZE(mx6q_sbc9000_spi_cs),
};

#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
static struct mtd_partition imx6_sbc9000_spi_nor_partitions[] = {
	{
	 .name = "bootloader",
	 .offset = 0,
	 .size = 0x00100000,
	},
	{
	 .name = "kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,
	},
};

static struct flash_platform_data imx6_sbc9000__spi_flash_data = {
	.name = "m25p80",
	.parts = imx6_sbc9000_spi_nor_partitions,
	.nr_parts = ARRAY_SIZE(imx6_sbc9000_spi_nor_partitions),
	.type = "sst25vf016b",
};
#endif

static struct ads7846_platform_data ads7846_config = {
        .x_max                  = 0x0fff,
        .y_max                  = 0x0fff,
//      .x_plate_ohms           = 180,
//      .pressure_max           = 255,
        .debounce_max           = 10,
        .debounce_tol           = 5,
        .debounce_rep           = 1,
        .gpio_pendown           = MX6Q_SBC9000_RES_TCH_INT,
        .keep_vref_on           = 1,
        .settle_delay_usecs     = 150,
        .wakeup                 = true,
        .swap_xy                = 1,
};

static struct spi_board_info imx6_sbc9000_spi_nor_device[] __initdata = {
#if 0
	{
		.modalias = "m25p80",
		.max_speed_hz = 20000000, /* max spi clock (SCK) speed in HZ */
		.bus_num = 0,
		.chip_select = 1,
		.platform_data = &imx6_sbc9000__spi_flash_data,
	},
#endif
	{
		.modalias = "ads7846",
		.bus_num = 0,
		.chip_select = 0,
		.max_speed_hz = 1500000,
		.irq = gpio_to_irq(MX6Q_SBC9000_RES_TCH_INT),
		.platform_data = &ads7846_config,
	},
};

static void spi_device_init(void)
{
	spi_register_board_info(imx6_sbc9000_spi_nor_device,
				ARRAY_SIZE(imx6_sbc9000_spi_nor_device));
}

static struct mxc_audio_platform_data mx6_sbc9000_audio_data;

static int mx6_sbc9000_sgtl5000_init(void)
{
	struct clk *clko;
	struct clk *new_parent;
	int rate;

	clko = clk_get(NULL, "clko_clk");
	if (IS_ERR(clko)) {
		pr_err("can't get CLKO clock.\n");
		return PTR_ERR(clko);
	}
#if 0 
	new_parent = clk_get(NULL, "ahb");
	if (!IS_ERR(new_parent)) {
		clk_set_parent(clko, new_parent);
		clk_put(new_parent);
	}
#endif
	rate = clk_round_rate(clko, 24000000);
	printk("rate = %d\n", rate);
	if (rate < 8000000 || rate > 27000000) {
		pr_err("Error:SGTL5000 mclk freq %d out of range!\n", rate);
		clk_put(clko);
		return -1;
	}

	mx6_sbc9000_audio_data.sysclk = rate;
	clk_set_rate(clko, rate);
	clk_enable(clko);
	return 0;
}

static struct imx_ssi_platform_data mx6_sbc9000_ssi_pdata = {
	.flags = IMX_SSI_DMA | IMX_SSI_SYN,
};

static struct mxc_audio_platform_data mx6_sbc9000_audio_data = {
	.ssi_num = 1,
	.src_port = 2,
	.ext_port = 3,
	.init = mx6_sbc9000_sgtl5000_init,
	.hp_gpio = -1,
};

static struct platform_device mx6_sbc9000_audio_device = {
	.name = "imx-sgtl5000",
};

static struct imxi2c_platform_data mx6q_sbc9000_i2c_data = {
	.bitrate = 400000,
};

static void mx6q_csi0_cam_powerdown(int powerdown)
{
	if (powerdown)
		gpio_set_value(MX6Q_SBC9000_CSI0_PWN, 1);
	else
		gpio_set_value(MX6Q_SBC9000_CSI0_PWN, 0);

	msleep(2);
}

static void mx6q_csi0_io_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(mx6q_sbc9000_csi0_sensor_pads,
			ARRAY_SIZE(mx6q_sbc9000_csi0_sensor_pads));

	/* Camera power down */
	gpio_request(MX6Q_SBC9000_CSI0_PWN, "cam-pwdn");
	gpio_direction_output(MX6Q_SBC9000_CSI0_PWN, 1);
	msleep(1);
	gpio_set_value(MX6Q_SBC9000_CSI0_PWN, 0);

	/* Camera reset */
	gpio_request(MX6Q_SBC9000_CSI0_RST, "cam-reset");
	gpio_direction_output(MX6Q_SBC9000_CSI0_RST, 1);

	gpio_set_value(MX6Q_SBC9000_CSI0_RST, 0);
	msleep(1);
	gpio_set_value(MX6Q_SBC9000_CSI0_RST, 1);

	/* For MX6Q GPR1 bit19 and bit20 meaning:
	 * Bit19:       0 - Enable mipi to IPU1 CSI0
	 *                      virtual channel is fixed to 0
	 *              1 - Enable parallel interface to IPU1 CSI0
	 * Bit20:       0 - Enable mipi to IPU2 CSI1
	 *                      virtual channel is fixed to 3
	 *              1 - Enable parallel interface to IPU2 CSI1
	 * IPU1 CSI1 directly connect to mipi csi2,
	 *      virtual channel is fixed to 1
	 * IPU2 CSI0 directly connect to mipi csi2,
	 *      virtual channel is fixed to 2
	 */
	mxc_iomux_set_gpr_register(1, 19, 1, 1);
}

static struct fsl_mxc_camera_platform_data camera_data = {
	.mclk = 24000000,
	.mclk_source = 0,
	.csi = 0,
	.io_init = mx6q_csi0_io_init,
	.pwdn = mx6q_csi0_cam_powerdown,
};

static struct i2c_board_info mxc_i2c0_board_info[] __initdata = {
    {
        I2C_BOARD_INFO("sgtl5000", 0x0a),
    },
    {
        I2C_BOARD_INFO("ov5640", 0x3c),
        .platform_data = (void *)&camera_data,
    },
    {
        I2C_BOARD_INFO("ov2656", 0x30),
        .platform_data = (void *)&camera_data,
    },
};

static struct i2c_board_info mxc_i2c1_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("mxc_hdmi_i2c", 0x50),
	},
};

static struct i2c_board_info mxc_i2c2_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("ch7033", 0x76),
	},

};

static void imx6q_sbc9000_usbotg_vbus(bool on)
{
	if (on)
		gpio_set_value(MX6Q_SBC9000_USB_OTG_PWR, 1);
	else
		gpio_set_value(MX6Q_SBC9000_USB_OTG_PWR, 0);
}

static void __init imx6q_sbc9000_init_usb(void)
{
	int ret = 0;

	imx_otg_base = MX6_IO_ADDRESS(MX6Q_USB_OTG_BASE_ADDR);
	/* disable external charger detect,
	 * or it will affect signal quality at dp .
	 */
	ret = gpio_request(MX6Q_SBC9000_USB_OTG_PWR, "usb-pwr");
	if (ret) {
		pr_err("failed to get GPIO MX6Q_SBC9000_USB_OTG_PWR: %d\n",
			ret);
		return;
	}
	gpio_direction_output(MX6Q_SBC9000_USB_OTG_PWR, 1);
	mxc_iomux_set_gpr_register(1, 13, 1, 0);

	mx6_set_otghost_vbus_func(imx6q_sbc9000_usbotg_vbus);
}

/* HW Initialization, if return 0, initialization is successful. */
static int mx6q_sbc9000_sata_init(struct device *dev, void __iomem *addr)
{
	u32 tmpdata;
	int ret = 0;
	struct clk *clk;

	sata_clk = clk_get(dev, "imx_sata_clk");
	if (IS_ERR(sata_clk)) {
		dev_err(dev, "no sata clock.\n");
		return PTR_ERR(sata_clk);
	}
	ret = clk_enable(sata_clk);
	if (ret) {
		dev_err(dev, "can't enable sata clock.\n");
		goto put_sata_clk;
	}

	/* Set PHY Paremeters, two steps to configure the GPR13,
	 * one write for rest of parameters, mask of first write is 0x07FFFFFD,
	 * and the other one write for setting the mpll_clk_off_b
	 *.rx_eq_val_0(iomuxc_gpr13[26:24]),
	 *.los_lvl(iomuxc_gpr13[23:19]),
	 *.rx_dpll_mode_0(iomuxc_gpr13[18:16]),
	 *.sata_speed(iomuxc_gpr13[15]),
	 *.mpll_ss_en(iomuxc_gpr13[14]),
	 *.tx_atten_0(iomuxc_gpr13[13:11]),
	 *.tx_boost_0(iomuxc_gpr13[10:7]),
	 *.tx_lvl(iomuxc_gpr13[6:2]),
	 *.mpll_ck_off(iomuxc_gpr13[1]),
	 *.tx_edgerate_0(iomuxc_gpr13[0]),
	 */
	tmpdata = readl(IOMUXC_GPR13);
	writel(((tmpdata & ~0x07FFFFFD) | 0x0593A044), IOMUXC_GPR13);

	/* enable SATA_PHY PLL */
	tmpdata = readl(IOMUXC_GPR13);
	writel(((tmpdata & ~0x2) | 0x2), IOMUXC_GPR13);

	/* Get the AHB clock rate, and configure the TIMER1MS reg later */
	clk = clk_get(NULL, "ahb");
	if (IS_ERR(clk)) {
		dev_err(dev, "no ahb clock.\n");
		ret = PTR_ERR(clk);
		goto release_sata_clk;
	}
	tmpdata = clk_get_rate(clk) / 1000;
	clk_put(clk);

#if defined (CONFIG_SATA_AHCI_PLATFORM) || defined(CONFIG_SATA_AHCI_PLATFORM_MODULE)
	ret = sata_init(addr, tmpdata);
	if (ret == 0)
		return ret;
#else
	usleep_range(1000, 2000);
	/* AHCI PHY enter into PDDQ mode if the AHCI module is not enabled */
	tmpdata = readl(addr + PORT_PHY_CTL);
	writel(tmpdata | PORT_PHY_CTL_PDDQ_LOC, addr + PORT_PHY_CTL);
	pr_info("No AHCI save PWR: PDDQ %s\n", ((readl(addr + PORT_PHY_CTL)
					>> 20) & 1) ? "enabled" : "disabled");
#endif

release_sata_clk:
	/* disable SATA_PHY PLL */
	writel((readl(IOMUXC_GPR13) & ~0x2), IOMUXC_GPR13);
	clk_disable(sata_clk);
put_sata_clk:
	clk_put(sata_clk);

	return ret;
}

#if defined (CONFIG_SATA_AHCI_PLATFORM) || defined(CONFIG_SATA_AHCI_PLATFORM_MODULE)
static void mx6q_sbc9000_sata_exit(struct device *dev)
{
	clk_disable(sata_clk);
	clk_put(sata_clk);
}

static struct ahci_platform_data mx6q_sbc9000_sata_data = {
	.init = mx6q_sbc9000_sata_init,
	.exit = mx6q_sbc9000_sata_exit,
};
#endif

#if 0
static struct gpio mx6q_sbc9000_flexcan_gpios[] = {
	{ MX6Q_SBC9000_CAN1_EN, GPIOF_OUT_INIT_LOW, "flexcan1-en" },
	{ MX6Q_SBC9000_CAN1_STBY, GPIOF_OUT_INIT_LOW, "flexcan1-stby" },
};

#endif

#if 1
static void mx6q_sbc9000_flexcan0_switch(int enable)
{
#if 0
	if (enable) {
		gpio_set_value(MX6Q_SBC9000_CAN1_EN, 1);
		gpio_set_value(MX6Q_SBC9000_CAN1_STBY, 1);
	} else {
		gpio_set_value(MX6Q_SBC9000_CAN1_EN, 0);
		gpio_set_value(MX6Q_SBC9000_CAN1_STBY, 0);
	}
#endif
}
#endif

static const struct flexcan_platform_data
	mx6q_sbc9000_flexcan0_pdata __initconst = {
	.transceiver_switch = mx6q_sbc9000_flexcan0_switch,
};

static const struct flexcan_platform_data
	mx6q_sbc9000_flexcan1_pdata __initconst = {
	.transceiver_switch = mx6q_sbc9000_flexcan0_switch,
};

static struct viv_gpu_platform_data imx6q_gpu_pdata __initdata = {
	.reserved_mem_size = SZ_128M + SZ_64M,
};

static struct imx_asrc_platform_data imx_asrc_data = {
	.channel_bits = 4,
	.clk_map_ver = 2,
};

static struct ipuv3_fb_platform_data sbc9000_fb_data[] = {
	{ /*fb0*/
	.disp_dev = "ldb",
	.interface_pix_fmt = IPU_PIX_FMT_RGB666,
	.mode_str = "LDB-XGA",
	.default_bpp = 32,
	.int_clk = false,
	}, {
	.disp_dev = "hdmi",
	.interface_pix_fmt = IPU_PIX_FMT_RGB24,
	.mode_str = "1920x1080M@60",
	.default_bpp = 32,
	.int_clk = false,
	}, {
	.disp_dev = "lcd",
	.interface_pix_fmt = IPU_PIX_FMT_RGB24,
	.mode_str = "7inch_LCD",
	.default_bpp = 32,
	.int_clk = false,
	}, {
	.disp_dev = "ldb",
	.interface_pix_fmt = IPU_PIX_FMT_RGB666,
	.mode_str = "LDB-VGA",
	.default_bpp = 16,
	.int_clk = false,
	},
};

static void hdmi_init(int ipu_id, int disp_id)
{
	int hdmi_mux_setting;

	if ((ipu_id > 1) || (ipu_id < 0)) {
		pr_err("Invalid IPU select for HDMI: %d. Set to 0\n", ipu_id);
		ipu_id = 0;
	}

	if ((disp_id > 1) || (disp_id < 0)) {
		pr_err("Invalid DI select for HDMI: %d. Set to 0\n", disp_id);
		disp_id = 0;
	}

	/* Configure the connection between IPU1/2 and HDMI */
	hdmi_mux_setting = 2*ipu_id + disp_id;

	/* GPR3, bits 2-3 = HDMI_MUX_CTL */
	mxc_iomux_set_gpr_register(3, 2, 2, hdmi_mux_setting);

	/* Set HDMI event as SDMA event2 while Chip version later than TO1.2 */
	if ((mx6q_revision() > IMX_CHIP_REVISION_1_1))
		mxc_iomux_set_gpr_register(0, 0, 1, 1);
}

/* On mx6x sbarelite board i2c2 iomux with hdmi ddc,
 * the pins default work at i2c2 function,
 when hdcp enable, the pins should work at ddc function */

static void hdmi_enable_ddc_pin(void)
{
	mxc_iomux_v3_setup_multiple_pads(mx6q_sbc9000_hdmi_ddc_pads,
		ARRAY_SIZE(mx6q_sbc9000_hdmi_ddc_pads));
}

static void hdmi_disable_ddc_pin(void)
{
	mxc_iomux_v3_setup_multiple_pads(mx6q_sbc9000_i2c2_pads,
		ARRAY_SIZE(mx6q_sbc9000_i2c2_pads));
}

static struct fsl_mxc_hdmi_platform_data hdmi_data = {
	.init = hdmi_init,
	.enable_pins = hdmi_enable_ddc_pin,
	.disable_pins = hdmi_disable_ddc_pin,
};

static struct fsl_mxc_hdmi_core_platform_data hdmi_core_data = {
	.ipu_id = 0,
	.disp_id = 0,
};

static struct fsl_mxc_lcd_platform_data lcdif_data = {
	.ipu_id = 0,
	.disp_id = 0,
	.default_ifmt = IPU_PIX_FMT_RGB24,
};

static struct fsl_mxc_ldb_platform_data ldb_data = {
	.ipu_id = 1,
	.disp_id = 0,
	.ext_ref = 1,
	.mode = LDB_SEP0,
	.sec_ipu_id = 1,
	.sec_disp_id = 1,
};

static void ldb_init(void)
{
        int ret;

        ret = gpio_request(MX6Q_SBC9000_LED_PWN, "led_pwn");
        if (ret) {
                pr_err("failed to get GPIO MX6Q_SBC9000_LED_PWN: %d\n",
                        ret);
                return;
        }

        gpio_direction_output(MX6Q_SBC9000_LED_PWN, 1);
}

static struct imx_ipuv3_platform_data ipu_data[] = {
	{
	.rev = 4,
	.csi_clk[0] = "clko2_clk",
	}, {
	.rev = 4,
	.csi_clk[0] = "clko2_clk",
	},
};

static struct ion_platform_data imx_ion_data = {
	.nr = 1,
	.heaps = {
		{
		.type = ION_HEAP_TYPE_CARVEOUT,
		.name = "vpu_ion",
		.size = SZ_16M,
		},
	},
};

static struct fsl_mxc_capture_platform_data capture_data[] = {
	{
		.csi = 0,
		.ipu = 0,
		.mclk_source = 0,
		.is_mipi = 0,
	}, {
		.csi = 1,
		.ipu = 0,
		.mclk_source = 0,
		.is_mipi = 1,
	},
};


struct imx_vout_mem {
	resource_size_t res_mbase;
	resource_size_t res_msize;
};

static struct imx_vout_mem vout_mem __initdata = {
	.res_msize = SZ_128M,
};


static void sbc9000_suspend_enter(void)
{
	/* suspend preparation */
}

static void sbc9000_suspend_exit(void)
{
	/* resume restore */
}
static const struct pm_platform_data mx6q_sbc9000_pm_data __initconst = {
	.name = "imx_pm",
	.suspend_enter = sbc9000_suspend_enter,
	.suspend_exit = sbc9000_suspend_exit,
};

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#define GPIO_BUTTON(gpio_num, ev_code, act_low, descr, wake)	\
{								\
	.gpio		= gpio_num,				\
	.type		= EV_KEY,				\
	.code		= ev_code,				\
	.active_low	= act_low,				\
	.desc		= "btn " descr,				\
	.wakeup		= wake,					\
}

static struct gpio_keys_button sbc9000_buttons[] = {
#if 0
	GPIO_BUTTON(MX6Q_SBC9000_ONOFF_KEY, KEY_POWER, 1, "key-power", 1),
	GPIO_BUTTON(MX6Q_SBC9000_MENU_KEY, KEY_MENU, 1, "key-memu", 0),
#endif
	GPIO_BUTTON(MX6Q_SBC9000_HOME_KEY, KEY_HOME, 1, "key-home", 0),
#if 0
	GPIO_BUTTON(MX6Q_SBC9000_BACK_KEY, KEY_BACK, 1, "key-back", 0),
	GPIO_BUTTON(MX6Q_SBC9000_VOL_UP_KEY, KEY_VOLUMEUP, 1, "volume-up", 0),
	GPIO_BUTTON(MX6Q_SBC9000_VOL_DOWN_KEY, KEY_VOLUMEDOWN, 1, "volume-down", 0),
#endif
};

static struct gpio_keys_platform_data sbc9000_button_data = {
	.buttons	= sbc9000_buttons,
	.nbuttons	= ARRAY_SIZE(sbc9000_buttons),
};

static struct platform_device sbc9000_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources  = 0,
	.dev		= {
		.platform_data = &sbc9000_button_data,
	}
};

static void __init sbc9000_add_device_buttons(void)
{
	platform_device_register(&sbc9000_button_device);
}
#else
static void __init sbc9000_add_device_buttons(void) {}
#endif

static struct gpio_led gpio_leds[] = {
        {
                .name                   = "sys_led",
                .default_trigger        = "heartbeat",
                .gpio                   = MX6Q_SBC9000_SYS_LED,
                .active_low             = true,
        },
        {
                .name                   = "user_led1",
                .gpio                   = MX6Q_SBC9000_USER1_LED,
                .active_low             = true,
        },
		{
                .name                   = "user_led2",
                .gpio                   = MX6Q_SBC9000_USER2_LED,
                .active_low             = true,
        },
};

static struct gpio_led_platform_data gpio_led_info = {
        .leds           = gpio_leds,
        .num_leds       = ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
        .name   = "leds-gpio",
        .id     = -1,
        .dev    = {
                .platform_data  = &gpio_led_info,
        },
};

static void __init sbc9000_add_device_leds(void)
{
        platform_device_register(&leds_gpio);
}

static struct regulator_consumer_supply sbc9000_vmmc_consumers[] = {
	REGULATOR_SUPPLY("vmmc", "sdhci-esdhc-imx.2"),
	REGULATOR_SUPPLY("vmmc", "sdhci-esdhc-imx.4"),
};

static struct regulator_init_data sbc9000_vmmc_init = {
	.num_consumer_supplies = ARRAY_SIZE(sbc9000_vmmc_consumers),
	.consumer_supplies = sbc9000_vmmc_consumers,
};

static struct fixed_voltage_config sbc9000_vmmc_reg_config = {
	.supply_name		= "vmmc",
	.microvolts		= 3300000,
	.gpio			= -1,
	.init_data		= &sbc9000_vmmc_init,
};

static struct platform_device sbc9000_vmmc_reg_devices = {
	.name	= "reg-fixed-voltage",
	.id	= 3,
	.dev	= {
		.platform_data = &sbc9000_vmmc_reg_config,
	},
};

#ifdef CONFIG_SND_SOC_SGTL5000

static struct regulator_consumer_supply sgtl5000_sbc9000_consumer_vdda = {
	.supply = "VDDA",
	.dev_name = "0-000a",
};

static struct regulator_consumer_supply sgtl5000_sbc9000_consumer_vddio = {
	.supply = "VDDIO",
	.dev_name = "0-000a",
};

static struct regulator_consumer_supply sgtl5000_sbc9000_consumer_vddd = {
	.supply = "VDDD",
	.dev_name = "0-000a",
};

static struct regulator_init_data sgtl5000_sbc9000_vdda_reg_initdata = {
	.num_consumer_supplies = 1,
	.consumer_supplies = &sgtl5000_sbc9000_consumer_vdda,
};

static struct regulator_init_data sgtl5000_sbc9000_vddio_reg_initdata = {
	.num_consumer_supplies = 1,
	.consumer_supplies = &sgtl5000_sbc9000_consumer_vddio,
};

static struct regulator_init_data sgtl5000_sbc9000_vddd_reg_initdata = {
	.num_consumer_supplies = 1,
	.consumer_supplies = &sgtl5000_sbc9000_consumer_vddd,
};

static struct fixed_voltage_config sgtl5000_sbc9000_vdda_reg_config = {
	.supply_name		= "VDDA",
	.microvolts		= 2500000,
	.gpio			= -1,
	.init_data		= &sgtl5000_sbc9000_vdda_reg_initdata,
};

static struct fixed_voltage_config sgtl5000_sbc9000_vddio_reg_config = {
	.supply_name		= "VDDIO",
	.microvolts		= 3300000,
	.gpio			= -1,
	.init_data		= &sgtl5000_sbc9000_vddio_reg_initdata,
};

static struct fixed_voltage_config sgtl5000_sbc9000_vddd_reg_config = {
	.supply_name		= "VDDD",
	.microvolts		= 0,
	.gpio			= -1,
	.init_data		= &sgtl5000_sbc9000_vddd_reg_initdata,
};

static struct platform_device sgtl5000_sbc9000_vdda_reg_devices = {
	.name	= "reg-fixed-voltage",
	.id	= 0,
	.dev	= {
		.platform_data = &sgtl5000_sbc9000_vdda_reg_config,
	},
};

static struct platform_device sgtl5000_sbc9000_vddio_reg_devices = {
	.name	= "reg-fixed-voltage",
	.id	= 1,
	.dev	= {
		.platform_data = &sgtl5000_sbc9000_vddio_reg_config,
	},
};

static struct platform_device sgtl5000_sbc9000_vddd_reg_devices = {
	.name	= "reg-fixed-voltage",
	.id	= 2,
	.dev	= {
		.platform_data = &sgtl5000_sbc9000_vddd_reg_config,
	},
};

#endif /* CONFIG_SND_SOC_SGTL5000 */

static int imx6q_init_audio(void)
{
	mxc_register_device(&mx6_sbc9000_audio_device,
			    &mx6_sbc9000_audio_data);
	imx6q_add_imx_ssi(1, &mx6_sbc9000_ssi_pdata);
#ifdef CONFIG_SND_SOC_SGTL5000
	platform_device_register(&sgtl5000_sbc9000_vdda_reg_devices);
	platform_device_register(&sgtl5000_sbc9000_vddio_reg_devices);
	platform_device_register(&sgtl5000_sbc9000_vddd_reg_devices);
#endif
	return 0;
}

static struct platform_pwm_backlight_data mx6_sbc9000_lcd_backlight_data = {
        .pwm_id = 1,
        .max_brightness = 255,
        .dft_brightness = 128,
        .pwm_period_ns = 50000,
};

static struct platform_pwm_backlight_data mx6_sbc9000_ldb_backlight_data = {
        .pwm_id = 0,
        .max_brightness = 255,
        .dft_brightness = 128,
        .pwm_period_ns = 50000,
};

static struct mxc_dvfs_platform_data sbc9000_dvfscore_data = {
	.reg_id = "cpu_vddgp",
	.soc_id = "cpu_vddsoc",
	.pu_id = "cpu_vddvpu",
	.clk1_id = "cpu_clk",
	.clk2_id = "gpc_dvfs_clk",
	.gpc_cntr_offset = MXC_GPC_CNTR_OFFSET,
	.ccm_cdcr_offset = MXC_CCM_CDCR_OFFSET,
	.ccm_cacrr_offset = MXC_CCM_CACRR_OFFSET,
	.ccm_cdhipr_offset = MXC_CCM_CDHIPR_OFFSET,
	.prediv_mask = 0x1F800,
	.prediv_offset = 11,
	.prediv_val = 3,
	.div3ck_mask = 0xE0000000,
	.div3ck_offset = 29,
	.div3ck_val = 2,
	.emac_val = 0x08,
	.upthr_val = 25,
	.dnthr_val = 9,
	.pncthr_val = 33,
	.upcnt_val = 10,
	.dncnt_val = 10,
	.delay_time = 80,
};

static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	char *str;
	struct tag *t;
	int i = 0;
	struct ipuv3_fb_platform_data *pdata_fb = sbc9000_fb_data;

	for_each_tag(t, tags) {
		if (t->hdr.tag == ATAG_CMDLINE) {
			str = t->u.cmdline.cmdline;
			str = strstr(str, "fbmem=");
			if (str != NULL) {
				str += 6;
				pdata_fb[i++].res_size[0] = memparse(str, &str);
				while (*str == ',' &&
					i < ARRAY_SIZE(sbc9000_fb_data)) {
					str++;
					pdata_fb[i++].res_size[0] = memparse(str, &str);
				}
			}
			/* GPU reserved memory */
			str = t->u.cmdline.cmdline;
			str = strstr(str, "gpumem=");
			if (str != NULL) {
				str += 7;
				imx6q_gpu_pdata.reserved_mem_size = memparse(str, &str);
			}
			break;
		}
	}
}

static struct mipi_csi2_platform_data mipi_csi2_pdata = {
	.ipu_id	 = 0,
	.csi_id = 0,
	.v_channel = 0,
	.lanes = 2,
	.dphy_clk = "mipi_pllref_clk",
	.pixel_clk = "emi_clk",
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource = {
	.name = "android ram console",
	.flags = IORESOURCE_MEM,
};

static struct platform_device android_ram_console = {
	.name = "ram_console",
	.num_resources = 1,
	.resource = &ram_console_resource,
};

static int __init imx6x_add_ram_console(void)
{
	return platform_device_register(&android_ram_console);
}
#else
#define imx6x_add_ram_console() do {} while (0)
#endif

static int __init caam_setup(char *__unused)
{
	caam_enabled = 1;
	return 1;
}
early_param("caam", caam_setup);

static const struct imx_pcie_platform_data mx6q_sbc9000_pcie_data __initconst = {
        .pcie_pwr_en    = -EINVAL,
        .pcie_rst       = MX6Q_SBC9000_PCIE_PERST,
        .pcie_wake_up   = MX6Q_SBC9000_PCIE_WAKE_B,
        .pcie_dis       = MX6Q_SBC9000_PCIE_W_DIS,
};

/*!
 * Board specific initialization.
 */
static void __init mx6_sbc9000_board_init(void)
{
	int i;
	int ret;
	struct clk *clko,*clko2;
	struct clk *new_parent;
	int rate;
	struct platform_device *voutdev;

	mxc_iomux_v3_setup_multiple_pads(mx6q_sbc9000_pads,
					ARRAY_SIZE(mx6q_sbc9000_pads));

#if 0
	if (enet_to_gpio_6) {
		iomux_v3_cfg_t enet_gpio_pad =
			MX6Q_PAD_GPIO_6__ENET_IRQ_TO_GPIO_6;
		mxc_iomux_v3_setup_pad(enet_gpio_pad);
	} else {
		/* J5 - Camera GP */
		iomux_v3_cfg_t camera_gpio_pad =
			MX6Q_PAD_GPIO_6__GPIO_1_6;
		mxc_iomux_v3_setup_pad(camera_gpio_pad);
	}
#endif

#ifdef CONFIG_FEC_1588
	/* Set GPIO_16 input for IEEE-1588 ts_clk and RMII reference clock
	 * For MX6 GPR1 bit21 meaning:
	 * Bit21:       0 - GPIO_16 pad output
	 *              1 - GPIO_16 pad input
	 */
	mxc_iomux_set_gpr_register(1, 21, 1, 1);
#endif

	gp_reg_id = sbc9000_dvfscore_data.reg_id;
	soc_reg_id = sbc9000_dvfscore_data.soc_id;
	pu_reg_id = sbc9000_dvfscore_data.pu_id;
	mx6q_sbc9000_init_uart();
	imx6q_add_mxc_hdmi_core(&hdmi_core_data);

	imx6q_add_ipuv3(0, &ipu_data[0]);
	imx6q_add_ipuv3(1, &ipu_data[1]);

	for (i = 0; i < ARRAY_SIZE(sbc9000_fb_data); i++)
		imx6q_add_ipuv3fb(i, &sbc9000_fb_data[i]);

	imx6q_add_vdoa();
	imx6q_add_lcdif(&lcdif_data);
	imx6q_add_ldb(&ldb_data);
	voutdev = imx6q_add_v4l2_output(0);
	if (vout_mem.res_msize && voutdev) {
		dma_declare_coherent_memory(&voutdev->dev,
					    vout_mem.res_mbase,
					    vout_mem.res_mbase,
					    vout_mem.res_msize,
					    (DMA_MEMORY_MAP |
					     DMA_MEMORY_EXCLUSIVE));
	}


	imx6q_add_v4l2_capture(0, &capture_data[0]);
	imx6q_add_v4l2_capture(1, &capture_data[1]);
	imx6q_add_mipi_csi2(&mipi_csi2_pdata);
	imx6q_add_imx_snvs_rtc();

	ldb_init();

	if (1 == caam_enabled)
		imx6q_add_imx_caam();

	imx6q_add_imx_i2c(0, &mx6q_sbc9000_i2c_data);
	imx6q_add_imx_i2c(1, &mx6q_sbc9000_i2c_data);
	imx6q_add_imx_i2c(2, &mx6q_sbc9000_i2c_data);
	i2c_register_board_info(0, mxc_i2c0_board_info,
			ARRAY_SIZE(mxc_i2c0_board_info));
	i2c_register_board_info(1, mxc_i2c1_board_info,
			ARRAY_SIZE(mxc_i2c1_board_info));
	i2c_register_board_info(2, mxc_i2c2_board_info,
			ARRAY_SIZE(mxc_i2c2_board_info));

	/* SPI */
	imx6q_add_ecspi(0, &mx6q_sbc9000_spi_data);
	spi_device_init();

	imx6q_add_mxc_hdmi(&hdmi_data);

	imx6q_add_anatop_thermal_imx(1, &mx6q_sbc9000_anatop_thermal_data);
#if 0
	if (enet_to_gpio_6)
		/* Make sure the IOMUX_OBSRV_MUX1 is set to ENET_IRQ. */
		mxc_iomux_set_specialbits_register(
			IOMUX_OBSRV_MUX1_OFFSET,
			OBSRV_MUX1_ENET_IRQ,
			OBSRV_MUX1_MASK);
	else
		fec_data.gpio_irq = -1;
#endif

	imx6_init_fec(fec_data);
#ifdef CONFIG_MX6_ENET_IRQ_TO_GPIO
	/* Make sure the IOMUX_OBSRV_MUX1 is set to ENET_IRQ. */
	mxc_iomux_set_specialbits_register(IOMUX_OBSRV_MUX1_OFFSET,
		OBSRV_MUX1_ENET_IRQ, OBSRV_MUX1_MASK);
#endif
	imx6q_add_pm_imx(0, &mx6q_sbc9000_pm_data);
	imx6q_add_sdhci_usdhc_imx(3, &mx6q_sbc9000_sd4_data);
	imx6q_add_sdhci_usdhc_imx(1, &mx6q_sbc9000_sd2_data);
	imx_add_viv_gpu(&imx6_gpu_data, &imx6q_gpu_pdata);
	imx6q_sbc9000_init_usb();

	if (cpu_is_mx6q()) {
#if defined (CONFIG_SATA_AHCI_PLATFORM) || defined(CONFIG_SATA_AHCI_PLATFORM_MODULE)
		imx6q_add_ahci(0, &mx6q_sbc9000_sata_data);
#else
		mx6q_sbc9000_sata_init(NULL,
			(void __iomem *)ioremap(MX6Q_SATA_BASE_ADDR, SZ_4K));
#endif
	}
	imx6q_add_vpu();
	imx6q_init_audio();
	platform_device_register(&sbc9000_vmmc_reg_devices);
	imx_asrc_data.asrc_core_clk = clk_get(NULL, "asrc_clk");
	imx_asrc_data.asrc_audio_clk = clk_get(NULL, "asrc_serial_clk");
	imx6q_add_asrc(&imx_asrc_data);

	/* release USB Hub reset */
	gpio_set_value(MX6Q_SBC9000_USB_HUB_RESET, 1);

	imx6q_add_mxc_pwm(0);
	imx6q_add_mxc_pwm(1);
	imx6q_add_mxc_pwm(2);
	imx6q_add_mxc_pwm(3);
	imx6q_add_mxc_pwm_backlight(1, &mx6_sbc9000_lcd_backlight_data);
	imx6q_add_mxc_pwm_backlight(0, &mx6_sbc9000_ldb_backlight_data);

	imx6q_add_otp();
	imx6q_add_viim();
	imx6q_add_imx2_wdt(0, NULL);
	imx6q_add_dma();

	imx6q_add_dvfs_core(&sbc9000_dvfscore_data);

	imx6q_add_ion(0, &imx_ion_data,
		sizeof(imx_ion_data) + sizeof(struct ion_platform_heap));

	sbc9000_add_device_buttons();
	sbc9000_add_device_leds();

	imx6q_add_hdmi_soc();
	imx6q_add_hdmi_soc_dai();

#if 0
	ret = gpio_request_array(mx6q_sbc9000_flexcan_gpios,
			ARRAY_SIZE(mx6q_sbc9000_flexcan_gpios));
	if (ret)
		pr_err("failed to request flexcan1-gpios: %d\n", ret);
	else
		imx6q_add_flexcan0(&mx6q_sbc9000_flexcan0_pdata);
#endif

	imx6q_add_flexcan0(&mx6q_sbc9000_flexcan0_pdata);
	imx6q_add_flexcan1(&mx6q_sbc9000_flexcan1_pdata);

	clko2 = clk_get(NULL, "clko2_clk");
	if (IS_ERR(clko2))
		pr_err("can't get CLKO2 clock.\n");

	new_parent = clk_get(NULL, "osc_clk");
	if (!IS_ERR(new_parent)) {
		clk_set_parent(clko2, new_parent);
		clk_put(new_parent);
	}
	rate = clk_round_rate(clko2, 24000000);
	clk_set_rate(clko2, rate);
	clk_enable(clko2);

    /* Camera and audio use osc clock */
    clko = clk_get(NULL, "clko_clk");
    if (!IS_ERR(clko))
        clk_set_parent(clko, clko2);

	imx6q_add_busfreq();

	/* Add PCIe RC interface support */
	imx6q_add_pcie(&mx6q_sbc9000_pcie_data);

	imx6q_add_perfmon(0);
	imx6q_add_perfmon(1);
	imx6q_add_perfmon(2);
}

extern void __iomem *twd_base;
static void __init mx6_sbc9000_timer_init(void)
{
	struct clk *uart_clk;
#ifdef CONFIG_LOCAL_TIMERS
	twd_base = ioremap(LOCAL_TWD_ADDR, SZ_256);
	BUG_ON(!twd_base);
#endif
	mx6_clocks_init(32768, 24000000, 0, 0);

	uart_clk = clk_get_sys("imx-uart.0", NULL);
	early_console_setup(UART2_BASE_ADDR, uart_clk);
}

static struct sys_timer mx6_sbc9000_timer = {
	.init   = mx6_sbc9000_timer_init,
};

static void __init mx6q_sbc9000_reserve(void)
{
	phys_addr_t phys;
	int i;

#ifdef CONFIG_ANDROID_RAM_CONSOLE
	phys = memblock_alloc_base(SZ_1M, SZ_4K, SZ_1G);
	memblock_remove(phys, SZ_1M);
	memblock_free(phys, SZ_1M);
	ram_console_resource.start = phys;
	ram_console_resource.end   = phys + SZ_1M - 1;
#endif

#if defined(CONFIG_MXC_GPU_VIV) || defined(CONFIG_MXC_GPU_VIV_MODULE)
	if (imx6q_gpu_pdata.reserved_mem_size) {
		phys = memblock_alloc_base(imx6q_gpu_pdata.reserved_mem_size,
					   SZ_4K, SZ_1G);
		memblock_remove(phys, imx6q_gpu_pdata.reserved_mem_size);
		imx6q_gpu_pdata.reserved_mem_base = phys;
	}
#endif

#if defined(CONFIG_ION)
	if (imx_ion_data.heaps[0].size) {
		phys = memblock_alloc(imx_ion_data.heaps[0].size, SZ_4K);
		memblock_remove(phys, imx_ion_data.heaps[0].size);
		imx_ion_data.heaps[0].base = phys;
	}
#endif

	for (i = 0; i < ARRAY_SIZE(sbc9000_fb_data); i++)
		if (sbc9000_fb_data[i].res_size[0]) {
			/* reserve for background buffer */
			phys = memblock_alloc(sbc9000_fb_data[i].res_size[0],
						SZ_4K);
			memblock_remove(phys, sbc9000_fb_data[i].res_size[0]);
			sbc9000_fb_data[i].res_base[0] = phys;
		}
	if (vout_mem.res_msize) {
		phys = memblock_alloc_base(vout_mem.res_msize,
					   SZ_4K, SZ_1G);
		memblock_remove(phys, vout_mem.res_msize);
		vout_mem.res_mbase = phys;
	}

}

/*
 * initialize __mach_desc_MX6Q_SBC9000 data structure.
 */
MACHINE_START(MX6Q_SBC9000, "Freescale i.MX 6Quad SBC9000 Board")
	/* Maintainer: Freescale Semiconductor, Inc. */
	.boot_params = MX6_PHYS_OFFSET + 0x100,
	.fixup = fixup_mxc_board,
	.map_io = mx6_map_io,
	.init_irq = mx6_init_irq,
	.init_machine = mx6_sbc9000_board_init,
	.timer = &mx6_sbc9000_timer,
	.reserve = mx6q_sbc9000_reserve,
MACHINE_END
