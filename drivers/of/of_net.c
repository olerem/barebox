/*
 * OF helpers for network devices.
 *
 * This file is released under the GPLv2
 *
 * Initially copied out of arch/powerpc/kernel/prom_parse.c
 */
#include <common.h>
#include <net.h>
#include <of_net.h>
#include <linux/phy.h>

/**
 * It maps 'enum phy_interface_t' found in include/linux/phy.h
 * into the device tree binding of 'phy-mode', so that Ethernet
 * device driver can get phy interface from device tree.
 */
static const char *phy_modes[] = {
	[PHY_INTERFACE_MODE_NA]		= "",
	[PHY_INTERFACE_MODE_INTERNAL]	= "internal",
	[PHY_INTERFACE_MODE_MII]	= "mii",
	[PHY_INTERFACE_MODE_GMII]	= "gmii",
	[PHY_INTERFACE_MODE_SGMII]	= "sgmii",
	[PHY_INTERFACE_MODE_TBI]	= "tbi",
	[PHY_INTERFACE_MODE_REVMII]	= "rev-mii",
	[PHY_INTERFACE_MODE_RMII]	= "rmii",
	[PHY_INTERFACE_MODE_RGMII]	= "rgmii",
	[PHY_INTERFACE_MODE_RGMII_ID]	= "rgmii-id",
	[PHY_INTERFACE_MODE_RGMII_RXID]	= "rgmii-rxid",
	[PHY_INTERFACE_MODE_RGMII_TXID]	= "rgmii-txid",
	[PHY_INTERFACE_MODE_RTBI]	= "rtbi",
	[PHY_INTERFACE_MODE_SMII]	= "smii",
	[PHY_INTERFACE_MODE_XGMII]	= "xgmii",
	[PHY_INTERFACE_MODE_MOCA]	= "moca",
	[PHY_INTERFACE_MODE_QSGMII]	= "qsgmii",
	[PHY_INTERFACE_MODE_TRGMII]	= "trgmii",
	[PHY_INTERFACE_MODE_1000BASEX]	= "1000base-x",
	[PHY_INTERFACE_MODE_2500BASEX]	= "2500base-x",
	[PHY_INTERFACE_MODE_RXAUI]	= "rxaui",
	[PHY_INTERFACE_MODE_XAUI]	= "xaui",
	[PHY_INTERFACE_MODE_10GKR]	= "10gbase-kr",
};

/**
 * of_get_phy_mode - Get phy mode for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets phy interface string from property 'phy-mode',
 * and return its index in phy_modes table, or errno in error case.
 */
int of_get_phy_mode(struct device_node *np)
{
	const char *pm;
	int err, i;

	err = of_property_read_string(np, "phy-mode", &pm);
	if (err < 0)
		err = of_property_read_string(np, "phy-connection-type", &pm);
	if (err < 0)
		return err;

	for (i = 0; i < ARRAY_SIZE(phy_modes); i++)
		if (!strcmp(pm, phy_modes[i]))
			return i;

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_get_phy_mode);

/**
 * Search the device tree for the best MAC address to use.  'mac-address' is
 * checked first, because that is supposed to contain to "most recent" MAC
 * address. If that isn't set, then 'local-mac-address' is checked next,
 * because that is the default address.  If that isn't set, then the obsolete
 * 'address' is checked, just in case we're using an old device tree.
 *
 * Note that the 'address' property is supposed to contain a virtual address of
 * the register set, but some DTS files have redefined that property to be the
 * MAC address.
 *
 * All-zero MAC addresses are rejected, because those could be properties that
 * exist in the device tree, but were not set by U-Boot.  For example, the
 * DTS could define 'mac-address' and 'local-mac-address', with zero MAC
 * addresses.  Some older U-Boots only initialized 'local-mac-address'.  In
 * this case, the real MAC is in 'local-mac-address', and 'mac-address' exists
 * but is all zeros.
*/
const void *of_get_mac_address(struct device_node *np)
{
	const void *p;
	int len, i;
	const char *str[] = { "mac-address", "local-mac-address", "address" };

	for (i = 0; i < ARRAY_SIZE(str); i++) {
		p = of_get_property(np, str[i], &len);
		if (p && (len == 6) && is_valid_ether_addr(p))
			return p;
	}

	return NULL;
}
EXPORT_SYMBOL(of_get_mac_address);
