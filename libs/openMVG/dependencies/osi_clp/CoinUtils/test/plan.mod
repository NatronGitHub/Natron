/* plan.mod (taken from GLPK 4.41 distribution) */

var bin1, >= 0, <= 200;
var bin2, >= 0, <= 2500;
var bin3, >= 400, <= 800;
var bin4, >= 100, <= 700;
var bin5, >= 0, <= 1500;
var alum, >= 0;
var silicon, >= 0;

minimize

value: .03 * bin1 + .08 * bin2 + .17 * bin3 + .12 * bin4 + .15 * bin5 +
       .21 * alum + .38 * silicon;

subject to

yield: bin1 + bin2 + bin3 + bin4 + bin5 + alum + silicon = 2000;

fe: .15 * bin1 + .04 * bin2 + .02 * bin3 + .04 * bin4 + .02 * bin5 +
    .01 * alum + .03 * silicon <= 60;

cu: .03 * bin1 + .05 * bin2 + .08 * bin3 + .02 * bin4 + .06 * bin5 +
    .01 * alum <= 100;

mn: .02 * bin1 + .04 * bin2 + .01 * bin3 + .02 * bin4 + .02 * bin5
    <= 40;

mg: .02 * bin1 + .03 * bin2 + .01 * bin5 <= 30;

al: .70 * bin1 + .75 * bin2 + .80 * bin3 + .75 * bin4 + .80 * bin5 +
    .97 * alum >= 1500;

si: 250 <= .02 * bin1 + .06 * bin2 + .08 * bin3 + .12 * bin4 +
    .02 * bin5 + .01 * alum + .97 * silicon <= 300;

end;

/* eof */
