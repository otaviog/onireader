import matplotlib.pyplot as plt

import onireader as oni

if __name__ == '__main__':
    dev = oni.Device()
    dev.open('030.oni')

    for i in range(len(dev)):
        depth_img = dev.readDepth()
        plt.imshow(depth_img)
        plt.show()

