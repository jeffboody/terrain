#!/bin/bash
mkdir terrainv1
./maketerrain 83 -180 -83 180 terrainv1  | tee terrainv1/log.txt
