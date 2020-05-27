#!/bin/bash
mkdir terrainv2
./maketerrain 83 -180 -83 180 terrainv2  | tee terrainv2/log.txt
