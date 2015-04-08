/*
 * wm8727.c
 *
 *  Created on: 15-Oct-2009
 *      Author: neil.jones@imgtec.com
 *
 * Copyright (C) 2009 Imagination Technologies Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

/*
 * Note this is a simple chip with no configuration interface, sample rate is
 * determined automatically by examining the Master clock and Bit clock ratios
 */
#define WM8727_RATES  (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
			SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |\
			SNDRV_PCM_RATE_192000)


static struct snd_soc_dai_driver wm8727_dai = {
	.name = "wm8727-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = WM8727_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
};

static struct snd_soc_codec_driver soc_codec_dev_wm8727;

static __devinit int wm8727_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_wm8727, &wm8727_dai, 1);
}

static int __devexit wm8727_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver wm8727_codec_driver = {
	.driver = {
			.name = "wm8727-codec",
			.owner = THIS_MODULE,
	},

	.probe = wm8727_probe,
	.remove = __devexit_p(wm8727_remove),
};

static int __init wm8727_init(void)
{
	return platform_driver_register(&wm8727_codec_driver);
}
module_init(wm8727_init);

static void __exit wm8727_exit(void)
{
	platform_driver_unregister(&wm8727_codec_driver);
}
module_exit(wm8727_exit);

MODULE_DESCRIPTION("ASoC wm8727 driver");
MODULE_AUTHOR("Neil Jones");
MODULE_LICENSE("GPL");
