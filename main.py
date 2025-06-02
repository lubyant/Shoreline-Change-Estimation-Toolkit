from shorelinecalculator import Config, SCET
config = Config("iter_100000.pth")
scet = SCET(config)
# scet.method1("LakeZero", "output")
scet.method2("METHOD_SITE/4108604/4108604_2005.tif",
             "output_single")
