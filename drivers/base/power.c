#include <common.h>
#include <driver.h>
#include <errno.h>
#include <of.h>

#include <pm_domain.h>

#define genpd_status_on(genpd)		(genpd->status == GPD_STATE_ACTIVE)

static LIST_HEAD(gpd_list);

/**
 * pm_genpd_init - Initialize a generic I/O PM domain object.
 * @genpd: PM domain object to initialize.
 * @gov: PM domain governor to associate with the domain (may be NULL).
 * @is_off: Initial value of the domain's power_is_off field.
 *
 * Returns 0 on successful initialization, else a negative error code.
 */
int pm_genpd_init(struct generic_pm_domain *genpd, void *gov, bool is_off)
{
	if (IS_ERR_OR_NULL(genpd))
		return -EINVAL;

	genpd->status = is_off ? GPD_STATE_POWER_OFF : GPD_STATE_ACTIVE;

	list_add(&genpd->gpd_list_node, &gpd_list);

	return 0;
}
EXPORT_SYMBOL_GPL(pm_genpd_init);

/**
 * struct of_genpd_provider - PM domain provider registration structure
 * @link: Entry in global list of PM domain providers
 * @node: Pointer to device tree node of PM domain provider
 * @xlate: Provider-specific xlate callback mapping a set of specifier cells
 *         into a PM domain.
 * @data: context pointer to be passed into @xlate callback
 */
struct of_genpd_provider {
	struct list_head link;
	struct device_node *node;
	genpd_xlate_t xlate;
	void *data;
};

/* List of registered PM domain providers. */
static LIST_HEAD(of_genpd_providers);

static bool genpd_present(const struct generic_pm_domain *genpd)
{
	const struct generic_pm_domain *gpd;

	if (IS_ERR_OR_NULL(genpd))
		return false;

	list_for_each_entry(gpd, &gpd_list, gpd_list_node)
		if (gpd == genpd)
			return true;

	return false;
}

/**
 * genpd_xlate_simple() - Xlate function for direct node-domain mapping
 * @genpdspec: OF phandle args to map into a PM domain
 * @data: xlate function private data - pointer to struct generic_pm_domain
 *
 * This is a generic xlate function that can be used to model PM domains that
 * have their own device tree nodes. The private data of xlate function needs
 * to be a valid pointer to struct generic_pm_domain.
 */
static struct generic_pm_domain *genpd_xlate_simple(
					struct of_phandle_args *genpdspec,
					void *data)
{
	return data;
}

/**
 * genpd_add_provider() - Register a PM domain provider for a node
 * @np: Device node pointer associated with the PM domain provider.
 * @xlate: Callback for decoding PM domain from phandle arguments.
 * @data: Context pointer for @xlate callback.
 */
static int genpd_add_provider(struct device_node *np, genpd_xlate_t xlate,
			      void *data)
{
	struct of_genpd_provider *cp;

	cp = kzalloc(sizeof(*cp), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;

	cp->node  = np;
	cp->data  = data;
	cp->xlate = xlate;

	list_add(&cp->link, &of_genpd_providers);
	pr_debug("Added domain provider from %pOF\n", np);

	return 0;
}

/**
 * of_genpd_add_provider_simple() - Register a simple PM domain provider
 * @np: Device node pointer associated with the PM domain provider.
 * @genpd: Pointer to PM domain associated with the PM domain provider.
 */
int of_genpd_add_provider_simple(struct device_node *np,
				 struct generic_pm_domain *genpd)
{
	int ret = -EINVAL;

	if (!np || !genpd)
		return -EINVAL;

	if (genpd_present(genpd))
		ret = genpd_add_provider(np, genpd_xlate_simple, genpd);

	return ret;
}
EXPORT_SYMBOL_GPL(of_genpd_add_provider_simple);

/**
 * genpd_get_from_provider() - Look-up PM domain
 * @genpdspec: OF phandle args to use for look-up
 *
 * Looks for a PM domain provider under the node specified by @genpdspec and if
 * found, uses xlate function of the provider to map phandle args to a PM
 * domain.
 *
 * Returns a valid pointer to struct generic_pm_domain on success or ERR_PTR()
 * on failure.
 */
static struct generic_pm_domain *genpd_get_from_provider(
					struct of_phandle_args *genpdspec)
{
	struct generic_pm_domain *genpd = ERR_PTR(-ENOENT);
	struct of_genpd_provider *provider;

	if (!genpdspec)
		return ERR_PTR(-EINVAL);

	/* Check if we have such a provider in our array */
	list_for_each_entry(provider, &of_genpd_providers, link) {
		if (provider->node == genpdspec->np)
			genpd = provider->xlate(genpdspec, provider->data);
		if (!IS_ERR(genpd))
			break;
	}

	return genpd;
}

static int _genpd_power_on(struct generic_pm_domain *genpd, bool timed)
{
	if (!genpd->power_on)
		return 0;

	return genpd->power_on(genpd);
}

/**
 * genpd_power_on - Restore power to a given PM domain and its masters.
 * @genpd: PM domain to power up.
 * @depth: nesting count for lockdep.
 *
 * Restore power to @genpd and all of its masters so that it is possible to
 * resume a device belonging to it.
 */
static int genpd_power_on(struct generic_pm_domain *genpd, unsigned int depth)
{
	int ret;

	if (genpd_status_on(genpd))
		return 0;

	ret = _genpd_power_on(genpd, true);
	if (ret)
		return ret;

	genpd->status = GPD_STATE_ACTIVE;

	return 0;
}

static int __genpd_dev_pm_attach(struct device_d *dev, struct device_node *np,
				 unsigned int index, bool power_on)
{
	struct of_phandle_args pd_args;
	struct generic_pm_domain *pd;
	int ret;

	ret = of_parse_phandle_with_args(np, "power-domains",
				"#power-domain-cells", index, &pd_args);
	if (ret < 0)
		return ret;

	pd = genpd_get_from_provider(&pd_args);
	if (IS_ERR(pd)) {
		ret = PTR_ERR(pd);
		dev_dbg(dev, "%s() failed to find PM domain: %d\n",
			__func__, ret);
		/*
		 * Assume that missing genpds are unresolved
		 * dependency are report them as deferred
		 */
		return (ret == -ENOENT) ? -EPROBE_DEFER : ret;
	}

	dev_dbg(dev, "adding to PM domain %s\n", pd->name);

	if (power_on)
		ret = genpd_power_on(pd, 0);

	return ret ?: 1;
}

/**
 * genpd_dev_pm_attach - Attach a device to its PM domain using DT.
 * @dev: Device to attach.
 *
 * Parse device's OF node to find a PM domain specifier. If such is found,
 * attaches the device to retrieved pm_domain ops.
 *
 * Returns 1 on successfully attached PM domain, 0 when the device don't need a
 * PM domain or when multiple power-domains exists for it, else a negative error
 * code. Note that if a power-domain exists for the device, but it cannot be
 * found or turned on, then return -EPROBE_DEFER to ensure that the device is
 * not probed and to re-try again later.
 */
int genpd_dev_pm_attach(struct device_d *dev)
{
	if (!dev->device_node)
		return 0;

	/*
	 * Devices with multiple PM domains must be attached separately, as we
	 * can only attach one PM domain per device.
	 */
	if (of_count_phandle_with_args(dev->device_node, "power-domains",
				       "#power-domain-cells") != 1)
		return 0;

	return __genpd_dev_pm_attach(dev, dev->device_node, 0, true);
}
EXPORT_SYMBOL_GPL(genpd_dev_pm_attach);
