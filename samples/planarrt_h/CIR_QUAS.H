#define Wozmush sdvig__; filename__; freq__; @\\ 
#STEP\\ 
#PARM\\
 ANGLE_UNIT radian;\\
 FREQ_UNIT GHz;\\
 LONG_UNIT mm;\\
 TIME_UNIT ns;\\
 DELTA W_delta;\\
 TIME 0.; 10.;\\
 X_MIN  0.0;\\
 X_MAX L_box;\\
 Y_MIN  0.0;\\
 Y_MAX W_box;\\
 FREQ  freq__;\\
#END_PARM\\
#TOPOLOGY\\
 BLOCK 1;\\
 POLYGON_STAT METAL;\\
   L   0;   a1/2;\\
   L   a2;  a1/2;\\
   L   a2;  a1/2+W_delta;\\
 END_B\\
 BLOCK 2;\\
 POLYGON_STAT METAL;\\
   L   0;   -a1/2;\\
   L   a2;  -a1/2;\\
   L   a2;  -(a1/2+W_delta);\\
 END_B\\
 BLOCK 3;\\
  POLYGON_STAT METAL;\\
   L   a2;   a1/2;\\
   L   a2+l; a1/2+h;\\
   L   a2+l; a1/2+h+th;\\
  END_B\\
 BLOCK 4;\\
  POLYGON_STAT METAL;\\
   L   a2;   -a1/2;\\
   L   a2+l; -(a1/2+h);\\
   L   a2+l; -(a1/2+h+th);\\
  END_B\\
 BLOCK 10;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe);\\
   L   a2+l+th*sin(beta); a1/2+h+th*(1+cos(beta));\\
   L   a2+l;              a1/2+h+th;\\
  END_B\\
 BLOCK 9;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta);\\
   L   a2+l+(apert-th)*sin(beta); a1/2+h+th+(apert-th)*cos(beta);\\
   L   a2+l+ apert    *sin(beta); a1/2+h+th+ apert    *cos(beta);\\
  END_B\\
 BLOCK 8;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)             ; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)             ; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)-W_delta;\\
  END_B\\
 BLOCK 7;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)             ; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)             ; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+W_delta;\\
  END_B\\
 BLOCK 6;\\
  POLYGON_STAT METAL;\\
   L   0; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta);\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta)-W_delta;\\
  END_B\\
 BLOCK 5;\\
  POLYGON_STAT METAL;\\
   L  0; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta);\\
   L  a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta);\\
   L  a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta)+W_delta;\\
  END_B\\
 BLOCK 16;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe));\\
   L   a2+l+th*sin(beta); -(a1/2+h+th*(1+cos(beta)));\\
   L   a2+l;              -(a1/2+h+th);\\
  END_B\\
 BLOCK 15;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta));\\
   L   a2+l+(apert-th)*sin(beta); -(a1/2+h+th+(apert-th)*cos(beta));\\
   L   a2+l+ apert    *sin(beta); -(a1/2+h+th+ apert    *cos(beta));\\
  END_B\\
 BLOCK 14;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)             ; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)             ; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)-W_delta);\\
  END_B\\
 BLOCK 13;\\
  POLYGON_STAT METAL;\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)             ; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)             ; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+W_delta);\\
  END_B\\
 BLOCK 12;\\
  POLYGON_STAT METAL;\\
   L   0; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta));\\
   L   a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta));\\
  END_B\\
 BLOCK 11;\\
  POLYGON_STAT METAL;\\
   L  0; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta));\\
   L  a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta));\\
   L  a2+l+th*sin(beta)-l/cos(alfa)*cos(albe)+a1*sin(beta)-b1*cos(beta); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta)+W_delta);\\
  END_B\\
 BLOCK 17;\\
  INPUT_X    ; -a1/2; a1/2; 0; 500;\\
 END_B\\
BLOCK 18;\\
  INPUT_X ; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta); a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta); 500; 1000;\\
 END_B\\
BLOCK 19;\\
  INPUT_X ; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta)); -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta)); 500; 1000;\\
 END_B\\
 BLOCK 21;\\
  RECT_STAT ABSORBER; 4*W_delta; L_box-4*W_delta; 2*W_delta; 5*W_delta;\\
 END_B\\
 BLOCK 22;\\
  RECT_STAT ABSORBER; 4*W_delta; L_box-4*W_delta; W_box-5*W_delta; W_box-2*W_delta;\\
 END_B\\
 BLOCK 23;\\
  RECT_STAT ABSORBER; L_box-5*W_delta; L_box-2*W_delta; 2*W_delta; W_box-W_delta;\\
 END_B\\
 BLOCK 24;\\
  RECT_STAT ABSORBER; 2*W_delta; 5*W_delta; -W_box/2+2*W_delta; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta));\\
 END_B\\
 BLOCK 25;\\
  RECT_STAT ABSORBER; 2*W_delta; 5*W_delta; -(a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta)); -a1/2;\\
 END_B\\
 BLOCK 26;\\
  RECT_STAT ABSORBER; 2*W_delta; 5*W_delta; a1/2; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+b1*sin(beta);\\
 END_B\\
 BLOCK 27;\\
  RECT_STAT ABSORBER; 2*W_delta; 5*W_delta; a1/2+h+th*(1+cos(beta))+l/cos(alfa)*sin(albe)+a1*cos(beta)+b1*sin(beta); W_box/2;\\
 END_B\\
 BLOCK 28;\\
 RECT_STAT [prof_rct] (-2.0103349)/exp((sqrt(x*x+y*y)/230)^3); -R_vessel; L_box-(a2+l+R_vessel); -W_box/2+4*W_delta; W_box/2-4*W_delta;\\
\\ FILE prof_rct ; -R_vessel; L_box-(a2+l+R_vessel); -W_box/2+4*W_delta; W_box/2-4*W_delta;\\
 END_B\\
 BLOCK 20;\\
 CIRCLE_STAT amp*cos(tpg*r0*(f-f0+sdvig__))/exp((r-r0)/dr*(r-r0)/dr)/exp((f0-f+sdvig__)/df*(f0-f+sdvig__)/df); r0-2*dr; r0+2*dr; f0+sdvig__-2*df; f0+sdvig__+2*df;\\
 END_B\\
#END_TOPOLOGY\\
#LINK_LIST\\
 T 1; 0.0; W_box/2;\\
 T 2; 0.0; W_box/2;\\
 T 3; 0.0; W_box/2;\\
 T 4; 0.0; W_box/2;\\
 T 5; 0.0; W_box/2;\\
 T 6; 0.0; W_box/2;\\
 T 7; 0.0; W_box/2;\\
 T 8; 0.0; W_box/2;\\
 T 9; 0.0; W_box/2;\\
 T 10; 0.0; W_box/2;\\
 T 11; 0.0; W_box/2;\\
 T 12; 0.0; W_box/2;\\
 T 13; 0.0; W_box/2;\\
 T 14; 0.0; W_box/2;\\
 T 15; 0.0; W_box/2;\\
 T 16; 0.0; W_box/2;\\
 T 17; 0.0; W_box/2;\\
 T 18; 0.0; W_box/2;\\
 T 19; 0.0; W_box/2;\\
 T 21; 0.0; 0.0;\\
 T 22; 0.0; 0.0;\\
 T 23; 0.0; 0.0;\\
 T 24; 0.0; W_box/2;\\
 T 25; 0.0; W_box/2;\\
 T 26; 0.0; W_box/2;\\
 T 27; 0.0; W_box/2;\\
 T 20; R_vessel+l+a2; W_box/2;\\
 T 28; R_vessel+l+a2; W_box/2;\\
#END_LINK\\
#OUTPUT \\
 FILE filename__;\\
 FIELD_DISTRIBUTION_M filename__; freq__; 7; 8;\\
#END_OUTPUT\\
#END_STEP

