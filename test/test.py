"""Simple testing. Download the 030.oni file from
https://github.com/scenenn/scenenn to execute.
"""
import argparse

import matplotlib.pyplot as plt

import onireader as oni


def _main():
    parser = argparse.ArgumentParser(description="Simple testing")
    parser.add_argument("--noshow", action="store_true")

    args = parser.parse_args()

    dev = oni.Device()
    dev.open('030.oni')
    # plt.ion()
    for i in range(len(dev)):
        depth_img, depth_ts, depth_idx = dev.readDepth()
        rgb_img, rgb_ts, rgb_idx = dev.readColor()

        if args.noshow:
            continue

        plt.subplot(1, 2, 1)
        plt.imshow(depth_img)
        plt.axis('off')

        plt.subplot(1, 2, 2)
        plt.imshow(rgb_img)
        plt.axis('off')

        plt.show()


if __name__ == '__main__':
    _main()
