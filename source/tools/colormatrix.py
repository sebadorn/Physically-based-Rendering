#!/usr/bin/python

IlluminantC = [0.3101, 0.3162]
IlluminantD65 = [0.3127, 0.3291]
IlluminantE = [0.33333333, 0.33333333]

GAMMA_REC709 = 0.0

# xRed, yRed, xGreen, yGreen, xBlue, yBlue, White point, Gamma
csNTSC = [ 0.67, 0.33, 0.21, 0.71, 0.14, 0.08, IlluminantC, GAMMA_REC709 ]
csEBU = [ 0.64, 0.33, 0.29, 0.60, 0.15, 0.06, IlluminantD65, GAMMA_REC709 ]
csSMPTE = [ 0.630, 0.340, 0.310, 0.595, 0.155, 0.070, IlluminantD65, GAMMA_REC709 ]
csHDTV = [ 0.670, 0.330, 0.210, 0.710, 0.150, 0.060, IlluminantD65, GAMMA_REC709 ]
csCIE = [ 0.7355, 0.2645, 0.2658, 0.7243, 0.1669, 0.0085, IlluminantE, GAMMA_REC709 ]
csRec709 = [ 0.64, 0.33, 0.30, 0.60, 0.15, 0.06, IlluminantD65, GAMMA_REC709 ]

# choose a color system
cs = csCIE


xr = cs[0]
yr = cs[1]
zr = 1.0 - ( xr + yr )

xg = cs[2]
yg = cs[3]
zg = 1.0 - ( xg + yg )

xb = cs[4]
yb = cs[5]
zb = 1.0 - ( xb + yb )

xw = cs[6][0]
yw = cs[6][1]
zw = 1.0 - xw - yw

Xr = xr / yr
Xg = xg / yg
Xb = xb / yb

Zr = zr / yr
Zg = zg / yg
Zb = zb / yb


detN1 = Xr * ( Zb - Zg )
detN2 = Xg * ( Zb - Zr )
detN3 = Xb * ( Zg - Zr )
detNrecip = 1.0 / ( detN1 - detN2 + detN3 )

n00 = detNrecip * ( Zb - Zg )
n01 = detNrecip * ( Xb * Zg - Xg * Zb )
n02 = detNrecip * ( Xg - Xb )

n10 = detNrecip * ( Zr - Zb )
n11 = detNrecip * ( Xr * Zb - Xb * Zr )
n12 = detNrecip * ( Xb - Xr )

n20 = detNrecip * ( Zg - Zr )
n21 = detNrecip * ( Xg * Zr - Xr * Zg )
n22 = detNrecip * ( Xr - Xg )


sr = n00 * xw + n01 * yw + n02 * zw
sg = n10 * xw + n11 * yw + n12 * zw
sb = n20 * xw + n21 * yw + n22 * zw


m00 = sr * Xr
m01 = sg * Xg
m02 = sb * Xb

m10 = sr
m11 = sg
m12 = sb

m20 = sr * Zr
m21 = sg * Zg
m22 = sb * Zb


sMul = sr * sg * sb
detM1 = sMul * Xr * ( Zb - Zg )
detM2 = sMul * Xg * ( Zb - Zr )
detM3 = sMul * Xb * ( Zg - Zr )
detMrecip = 1.0 / ( detM1 - detM2 + detM3 )

m00inv = detMrecip * sg * sb * ( Zb - Zg )
m01inv = detMrecip * sg * sb * ( Xb * Zg - Xg * Zb )
m02inv = detMrecip * sg * sb * ( Xg - Xb )

m10inv = detMrecip * sb * sr * ( Zr - Zb )
m11inv = detMrecip * sb * sr * ( Xr * Zb - Xb * Zr )
m12inv = detMrecip * sb * sr * ( Xb - Xr )

m20inv = detMrecip * sg * sr * ( Zg - Zr )
m21inv = detMrecip * sg * sr * ( Xg * Zr - Xr * Zg )
m22inv = detMrecip * sg * sr * ( Xr - Xg )

print "%f %f %f" % ( m00inv, m01inv, m02inv )
print "%f %f %f" % ( m10inv, m11inv, m12inv )
print "%f %f %f" % ( m20inv, m21inv, m22inv )


## NTSC
#  6.040009 -1.683788 -0.911408
# -3.113923  6.322208 -0.089522
#  0.184473 -0.374537  2.839774

## EBU
#  9.313725 -4.236410 -1.446676
# -2.944388  5.698851  0.126237
#  0.206346 -0.695713  3.250795

## SMPTE
# 10.660419 -5.290040 -1.654274
# -3.247467  6.007939  0.106841
#  0.171210 -0.598937  3.192554

## HDTV
#  6.205850 -1.717461 -1.047886
# -2.715540  5.513369  0.096872
#  0.193850 -0.393574  2.984110

## CIE
#  6.863516 -2.500103 -1.363412
# -1.534954  4.268275  0.266679
#  0.017161 -0.047721  3.030559

## Rec709
#  9.854084 -4.674373 -1.516013
# -2.944388  5.698851  0.126237
#  0.169153 -0.620228  3.213911
