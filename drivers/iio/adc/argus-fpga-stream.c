// SPDX-License-Identifier: GPL-2.0
/*
 * ARGUS FPGA packet stream: an IIO device whose only data is a stream of
 * 32-bit words that an FPGA block sends through one AXI DMAC (AXI-Stream to
 * memory). The driver knows nothing of the packets: their format belongs to
 * the FPGA block and to user space (RnP: docs/spec/spectrum.md).
 *
 * It has no registers of its own. The DMA controller is driven by its own
 * driver (dma-axi-dmac); the IIO DMA buffer hands user space whole blocks
 * only, once the DMA has written them.
 *
 * Device tree:
 *
 *	spectrum@0 {
 *		compatible = "argus,fpga-stream-1.0";
 *		label = "rnp-spectrum";
 *		dmas = <&spectrum_dma 0>;
 *		dma-names = "rx";
 *	};
 */

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/buffer-dmaengine.h>

static const struct iio_chan_spec argus_fpga_stream_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.scan_index = 0,
		.scan_type = {
			.sign = 'u',
			.realbits = 32,
			.storagebits = 32,
			.shift = 0,
			.endianness = IIO_LE,
		},
	},
};

/* Only one channel: it is always the one in the buffer. */
static const unsigned long argus_fpga_stream_scan_masks[] = { BIT(0), 0 };

static const struct iio_info argus_fpga_stream_info = {
};

static int argus_fpga_stream_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, 0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "argus-fpga-stream";	/* "label" from the DT tells them apart */
	indio_dev->info = &argus_fpga_stream_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = argus_fpga_stream_channels;
	indio_dev->num_channels = ARRAY_SIZE(argus_fpga_stream_channels);
	indio_dev->available_scan_masks = argus_fpga_stream_scan_masks;

	ret = devm_iio_dmaengine_buffer_setup(&pdev->dev, indio_dev, "rx");
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "no DMA channel \"rx\"\n");

	return devm_iio_device_register(&pdev->dev, indio_dev);
}

static const struct of_device_id argus_fpga_stream_of_match[] = {
	{ .compatible = "argus,fpga-stream-1.0" },
	{ }
};
MODULE_DEVICE_TABLE(of, argus_fpga_stream_of_match);

static struct platform_driver argus_fpga_stream_driver = {
	.driver = {
		.name = "argus-fpga-stream",
		.of_match_table = argus_fpga_stream_of_match,
	},
	.probe = argus_fpga_stream_probe,
};
module_platform_driver(argus_fpga_stream_driver);

MODULE_DESCRIPTION("ARGUS FPGA packet stream through an AXI DMAC (IIO)");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(IIO_DMAENGINE_BUFFER);
