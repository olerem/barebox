/*
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>

struct ar2313_fmc {
	struct device_d *dev;
	void __iomem *base;
};

static int ar2313_fmc_parse_dt(struct ar2313_fmc *priv)
{
	int ret;

	printk("%s:%i\n", __func__, __LINE__);
	ret = of_platform_populate(priv->dev->device_node, of_default_bus_match_table, priv->dev);
	if (ret)
		dev_err(priv->dev, "%s fail to create devices.\n",
			priv->dev->device_node->full_name);
	return ret;
}

static int ar2313_fmc_probe(struct device_d *dev)
{
	struct ar2313_fmc *priv;
	struct resource *iores;
	int ret = 0;

	priv = xzalloc(sizeof(*priv));

	dev_info(dev, "%s:%i\n", __func__, __LINE__);
	priv->dev = dev;

	/* get the resource */
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		return ret;
	}
	priv->base = IOMEM(iores->start);

	dev_info(dev, "%s:%i\n", __func__, __LINE__);
	/* parse the device node */
	ret = ar2313_fmc_parse_dt(priv);
	if (ret)
		return ret;

	dev_info(dev, "%s:%i\n", __func__, __LINE__);
	dev_info(dev, "AR2313 Flash Memory Controller driver registered.\n");

	return 0;
}

static struct of_device_id ar2313_fmc_id_table[] = {
	{
		.compatible = "qca,ar2313-fmc",
	}
};

static struct driver_d ar2313_fmc_driver = {
	.name = "ar2313-fmc",
	.of_compatible = DRV_OF_COMPAT(ar2313_fmc_id_table),
	.probe   = ar2313_fmc_probe,
};
device_platform_driver(ar2313_fmc_driver);
