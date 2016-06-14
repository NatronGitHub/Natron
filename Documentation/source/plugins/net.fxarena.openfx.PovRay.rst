PovRayOFX
=========

.. figure:: net.fxarena.openfx.PovRay.png
   :alt: 

*This documentation is for version 1.2 of PovRayOFX.*

Persistence of Vision raytracer generic node.

POV-Ray is not included and must be installed (in PATH) prior to using this node.

https://github.com/olear/openfx-arena/wiki/PovRay for more information regarding usage and installation.

Powered by POV-Ray and ImageMagick.

POV-Ray is Copyright 2003-2015 Persistence of Vision Raytracer Pty. Ltd.

The terms "POV-Ray" and "Persistence of Vision Raytracer" are trademarks of Persistence of Vision Raytracer Pty. Ltd.

POV-Ray is distributed under the AGPL3 license.

ImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.

ImageMagick is distributed under the Apache 2.0 license.

Inputs & Controls
-----------------

+------+------+------+------+
| Labe | Scri | Defa | Func |
| l    | pt-N | ult- | tion |
| (UI  | ame  | Valu |      |
| Name |      | e    |      |
| )    |      |      |      |
+======+======+======+======+
| PovR | PovR |      |      |
| ayOF | ayOF |      |      |
| X    | X    |      |      |
+------+------+------+------+
| Canv | Canv |      |      |
| as   | as   |      |      |
+------+------+------+------+
| Adva | Adva |      |      |
| nced | nced |      |      |
+------+------+------+------+
| Scen | scen | //   | Add  |
| e    | e    | This | or   |
|      |      | work | writ |
|      |      | is   | e    |
|      |      | lice | your |
|      |      | nsed | POV- |
|      |      | unde | Ray  |
|      |      | r    | scen |
|      |      | the  | e    |
|      |      | Crea | here |
|      |      | tive | .    |
|      |      | Comm | Exam |
|      |      | ons  | ples |
|      |      | Attr | can  |
|      |      | ibut | be   |
|      |      | ion  | foun |
|      |      | 3.0  | d    |
|      |      | Unpo | at   |
|      |      | rted | http |
|      |      | Lice | s:// |
|      |      | nse. | gith |
|      |      | //   | ub.c |
|      |      | To   | om/P |
|      |      | view | OV-R |
|      |      | a    | ay/p |
|      |      | copy | ovra |
|      |      | of   | y/tr |
|      |      | this | ee/m |
|      |      | lice | aste |
|      |      | nse, | r/di |
|      |      | visi | stri |
|      |      | t    | buti |
|      |      | http | on/s |
|      |      | ://c | cene |
|      |      | reat | s    |
|      |      | ivec |      |
|      |      | ommo |      |
|      |      | ns.o |      |
|      |      | rg/l |      |
|      |      | icen |      |
|      |      | ses/ |      |
|      |      | by/3 |      |
|      |      | .0// |      |
|      |      | /    |      |
|      |      | or   |      |
|      |      | send |      |
|      |      | a    |      |
|      |      | lett |      |
|      |      | er   |      |
|      |      | to   |      |
|      |      | Crea |      |
|      |      | tive |      |
|      |      | Comm |      |
|      |      | ons, |      |
|      |      | 444  |      |
|      |      | Cast |      |
|      |      | ro   |      |
|      |      | Stre |      |
|      |      | et,  |      |
|      |      | Suit |      |
|      |      | e    |      |
|      |      | 900, |      |
|      |      | Moun |      |
|      |      | tain |      |
|      |      | View |      |
|      |      | ,//  |      |
|      |      | Cali |      |
|      |      | forn |      |
|      |      | ia,  |      |
|      |      | 9404 |      |
|      |      | 1,   |      |
|      |      | USA. |      |
|      |      | //   |      |
|      |      | Pers |      |
|      |      | iste |      |
|      |      | nce  |      |
|      |      | Of   |      |
|      |      | Visi |      |
|      |      | on   |      |
|      |      | rayt |      |
|      |      | race |      |
|      |      | r    |      |
|      |      | samp |      |
|      |      | le   |      |
|      |      | file |      |
|      |      | .//  |      |
|      |      | File |      |
|      |      | by   |      |
|      |      | Dan  |      |
|      |      | Farm |      |
|      |      | er// |      |
|      |      | Demo |      |
|      |      | nstr |      |
|      |      | ates |      |
|      |      | glas |      |
|      |      | s    |      |
|      |      | text |      |
|      |      | ures |      |
|      |      | ,    |      |
|      |      | CGS  |      |
|      |      | with |      |
|      |      | box  |      |
|      |      | prim |      |
|      |      | itiv |      |
|      |      | es,  |      |
|      |      | one  |      |
|      |      | of   |      |
|      |      | Mike |      |
|      |      | Mill |      |
|      |      | er's |      |
|      |      | //   |      |
|      |      | fabu |      |
|      |      | lous |      |
|      |      | marb |      |
|      |      | le   |      |
|      |      | text |      |
|      |      | ures |      |
|      |      | ,    |      |
|      |      | modi |      |
|      |      | fied |      |
|      |      | with |      |
|      |      | an   |      |
|      |      | octa |      |
|      |      | ves  |      |
|      |      | chan |      |
|      |      | ge,  |      |
|      |      | and  |      |
|      |      | does |      |
|      |      | n't/ |      |
|      |      | /    |      |
|      |      | make |      |
|      |      | a    |      |
|      |      | half |      |
|      |      | -bad |      |
|      |      | imag |      |
|      |      | e,   |      |
|      |      | eith |      |
|      |      | er.  |      |
|      |      | Inte |      |
|      |      | rest |      |
|      |      | ing  |      |
|      |      | ligh |      |
|      |      | ting |      |
|      |      | effe |      |
|      |      | ct,  |      |
|      |      | too. |      |
|      |      | #ver |      |
|      |      | sion |      |
|      |      | 3.7; |      |
|      |      | glob |      |
|      |      | al\_ |      |
|      |      | sett |      |
|      |      | ings |      |
|      |      | {ass |      |
|      |      | umed |      |
|      |      | \_ga |      |
|      |      | mma  |      |
|      |      | 2.0m |      |
|      |      | ax\_ |      |
|      |      | trac |      |
|      |      | e\_l |      |
|      |      | evel |      |
|      |      | 5}#i |      |
|      |      | nclu |      |
|      |      | de   |      |
|      |      | "col |      |
|      |      | ors. |      |
|      |      | inc" |      |
|      |      | #inc |      |
|      |      | lude |      |
|      |      | "sha |      |
|      |      | pes. |      |
|      |      | inc" |      |
|      |      | #inc |      |
|      |      | lude |      |
|      |      | "tex |      |
|      |      | ture |      |
|      |      | s.in |      |
|      |      | c"#i |      |
|      |      | nclu |      |
|      |      | de   |      |
|      |      | "gla |      |
|      |      | ss.i |      |
|      |      | nc"# |      |
|      |      | incl |      |
|      |      | ude  |      |
|      |      | "sto |      |
|      |      | nes. |      |
|      |      | inc" |      |
|      |      | came |      |
|      |      | ra   |      |
|      |      | {loc |      |
|      |      | atio |      |
|      |      | n    |      |
|      |      | <0.7 |      |
|      |      | 5,   |      |
|      |      | 3.5, |      |
|      |      | -3.5 |      |
|      |      | >ang |      |
|      |      | le   |      |
|      |      | 100  |      |
|      |      | //   |      |
|      |      | dire |      |
|      |      | ctio |      |
|      |      | n    |      |
|      |      | <0.0 |      |
|      |      | ,    |      |
|      |      | 0.0, |      |
|      |      | 0.5> |      |
|      |      | //   |      |
|      |      | wide |      |
|      |      | -ang |      |
|      |      | le   |      |
|      |      | view |      |
|      |      | righ |      |
|      |      | t    |      |
|      |      | x\ * |      |
|      |      | imag |      |
|      |      | e\_w |      |
|      |      | idth |      |
|      |      | /ima |      |
|      |      | ge\_ |      |
|      |      | heig |      |
|      |      | htlo |      |
|      |      | ok\_ |      |
|      |      | at   |      |
|      |      | <0,  |      |
|      |      | 0,   |      |
|      |      | -1>} |      |
|      |      | //   |      |
|      |      | Ligh |      |
|      |      | t    |      |
|      |      | sour |      |
|      |      | ces, |      |
|      |      | two  |      |
|      |      | to   |      |
|      |      | the  |      |
|      |      | fron |      |
|      |      | t,   |      |
|      |      | righ |      |
|      |      | t,   |      |
|      |      | on   |      |
|      |      | from |      |
|      |      | the  |      |
|      |      | left |      |
|      |      | ,    |      |
|      |      | rear |      |
|      |      | .lig |      |
|      |      | ht\_ |      |
|      |      | sour |      |
|      |      | ce   |      |
|      |      | {<-3 |      |
|      |      | 0,   |      |
|      |      | 11,  |      |
|      |      | 20>  |      |
|      |      | colo |      |
|      |      | r    |      |
|      |      | Whit |      |
|      |      | e}li |      |
|      |      | ght\ |      |
|      |      | _sou |      |
|      |      | rce  |      |
|      |      | {<   |      |
|      |      | 31,  |      |
|      |      | 12,  |      |
|      |      | -20> |      |
|      |      | colo |      |
|      |      | r    |      |
|      |      | Whit |      |
|      |      | e}li |      |
|      |      | ght\ |      |
|      |      | _sou |      |
|      |      | rce  |      |
|      |      | {<   |      |
|      |      | 32,  |      |
|      |      | 11,  |      |
|      |      | -20> |      |
|      |      | colo |      |
|      |      | r    |      |
|      |      | Ligh |      |
|      |      | tGra |      |
|      |      | y}un |      |
|      |      | ion  |      |
|      |      | {//  |      |
|      |      | A    |      |
|      |      | gree |      |
|      |      | n    |      |
|      |      | glas |      |
|      |      | s    |      |
|      |      | ball |      |
|      |      | insi |      |
|      |      | de   |      |
|      |      | of a |      |
|      |      | box- |      |
|      |      | shap |      |
|      |      | ed   |      |
|      |      | fram |      |
|      |      | esph |      |
|      |      | ere  |      |
|      |      | {    |      |
|      |      | <0,  |      |
|      |      | 0,   |      |
|      |      | 0>,  |      |
|      |      | 1.75 |      |
|      |      | //   |      |
|      |      | conv |      |
|      |      | erte |      |
|      |      | d    |      |
|      |      | to   |      |
|      |      | mate |      |
|      |      | rial |      |
|      |      | 07Au |      |
|      |      | g200 |      |
|      |      | 8    |      |
|      |      | (jh) |      |
|      |      | mate |      |
|      |      | rial |      |
|      |      | {tex |      |
|      |      | ture |      |
|      |      | {pig |      |
|      |      | ment |      |
|      |      | {col |      |
|      |      | or   |      |
|      |      | gree |      |
|      |      | n    |      |
|      |      | 0.90 |      |
|      |      | filt |      |
|      |      | er   |      |
|      |      | 0.85 |      |
|      |      | }fin |      |
|      |      | ish  |      |
|      |      | {pho |      |
|      |      | ng   |      |
|      |      | 1    |      |
|      |      | phon |      |
|      |      | g\_s |      |
|      |      | ize  |      |
|      |      | 300  |      |
|      |      | //   |      |
|      |      | Very |      |
|      |      | tigh |      |
|      |      | t    |      |
|      |      | high |      |
|      |      | ligh |      |
|      |      | tsre |      |
|      |      | flec |      |
|      |      | tion |      |
|      |      | 0.15 |      |
|      |      | //   |      |
|      |      | Need |      |
|      |      | s    |      |
|      |      | a    |      |
|      |      | litt |      |
|      |      | le   |      |
|      |      | refl |      |
|      |      | ecti |      |
|      |      | on   |      |
|      |      | adde |      |
|      |      | d}}i |      |
|      |      | nter |      |
|      |      | ior{ |      |
|      |      | caus |      |
|      |      | tics |      |
|      |      | 1.0i |      |
|      |      | or   |      |
|      |      | 1.5} |      |
|      |      | }}// |      |
|      |      | A    |      |
|      |      | box- |      |
|      |      | shap |      |
|      |      | ed   |      |
|      |      | fram |      |
|      |      | e    |      |
|      |      | surr |      |
|      |      | ound |      |
|      |      | ing  |      |
|      |      | a    |      |
|      |      | gree |      |
|      |      | n    |      |
|      |      | glas |      |
|      |      | s    |      |
|      |      | ball |      |
|      |      | diff |      |
|      |      | eren |      |
|      |      | ce   |      |
|      |      | {obj |      |
|      |      | ect  |      |
|      |      | {Uni |      |
|      |      | tBox |      |
|      |      | scal |      |
|      |      | e    |      |
|      |      | 1.5} |      |
|      |      | //   |      |
|      |      | The  |      |
|      |      | outs |      |
|      |      | ide  |      |
|      |      | dime |      |
|      |      | nsio |      |
|      |      | ns// |      |
|      |      | And  |      |
|      |      | some |      |
|      |      | squa |      |
|      |      | re   |      |
|      |      | hole |      |
|      |      | s    |      |
|      |      | in   |      |
|      |      | all  |      |
|      |      | side |      |
|      |      | s.   |      |
|      |      | Note |      |
|      |      | that |      |
|      |      | each |      |
|      |      | of// |      |
|      |      | thes |      |
|      |      | e    |      |
|      |      | boxe |      |
|      |      | s    |      |
|      |      | that |      |
|      |      | are  |      |
|      |      | goin |      |
|      |      | g    |      |
|      |      | to   |      |
|      |      | be   |      |
|      |      | subt |      |
|      |      | ract |      |
|      |      | ed   |      |
|      |      | has  |      |
|      |      | one  |      |
|      |      | vect |      |
|      |      | or// |      |
|      |      | scal |      |
|      |      | ed   |      |
|      |      | just |      |
|      |      | slig |      |
|      |      | htly |      |
|      |      | larg |      |
|      |      | er   |      |
|      |      | than |      |
|      |      | the  |      |
|      |      | outs |      |
|      |      | ide  |      |
|      |      | box. |      |
|      |      | The  |      |
|      |      | othe |      |
|      |      | r//  |      |
|      |      | two  |      |
|      |      | vect |      |
|      |      | ors  |      |
|      |      | dete |      |
|      |      | rmin |      |
|      |      | e    |      |
|      |      | the  |      |
|      |      | size |      |
|      |      | of   |      |
|      |      | the  |      |
|      |      | hole |      |
|      |      | .//  |      |
|      |      | Clip |      |
|      |      | some |      |
|      |      | sqr  |      |
|      |      | hole |      |
|      |      | s    |      |
|      |      | in   |      |
|      |      | the  |      |
|      |      | box  |      |
|      |      | to   |      |
|      |      | make |      |
|      |      | a 3D |      |
|      |      | box  |      |
|      |      | fram |      |
|      |      | eobj |      |
|      |      | ect{ |      |
|      |      | Unit |      |
|      |      | Box  |      |
|      |      | scal |      |
|      |      | e    |      |
|      |      | <1.5 |      |
|      |      | 1,   |      |
|      |      | 1.25 |      |
|      |      | ,    |      |
|      |      | 1.25 |      |
|      |      | >    |      |
|      |      | } // |      |
|      |      | clip |      |
|      |      | xobj |      |
|      |      | ect{ |      |
|      |      | Unit |      |
|      |      | Box  |      |
|      |      | scal |      |
|      |      | e    |      |
|      |      | <1.2 |      |
|      |      | 5,   |      |
|      |      | 1.51 |      |
|      |      | ,    |      |
|      |      | 1.25 |      |
|      |      | >    |      |
|      |      | } // |      |
|      |      | clip |      |
|      |      | yobj |      |
|      |      | ect{ |      |
|      |      | Unit |      |
|      |      | Box  |      |
|      |      | scal |      |
|      |      | e    |      |
|      |      | <1.2 |      |
|      |      | 5,   |      |
|      |      | 1.25 |      |
|      |      | ,    |      |
|      |      | 1.51 |      |
|      |      | >    |      |
|      |      | } // |      |
|      |      | clip |      |
|      |      | zpig |      |
|      |      | ment |      |
|      |      | {    |      |
|      |      | red  |      |
|      |      | 0.75 |      |
|      |      | gree |      |
|      |      | n    |      |
|      |      | 0.75 |      |
|      |      | blue |      |
|      |      | 0.85 |      |
|      |      | }fin |      |
|      |      | ish  |      |
|      |      | {amb |      |
|      |      | ient |      |
|      |      | 0.2d |      |
|      |      | iffu |      |
|      |      | se   |      |
|      |      | 0.7r |      |
|      |      | efle |      |
|      |      | ctio |      |
|      |      | n    |      |
|      |      | 0.15 |      |
|      |      | bril |      |
|      |      | lian |      |
|      |      | ce   |      |
|      |      | 8spe |      |
|      |      | cula |      |
|      |      | r    |      |
|      |      | 1rou |      |
|      |      | ghne |      |
|      |      | ss   |      |
|      |      | 0.01 |      |
|      |      | }//  |      |
|      |      | Same |      |
|      |      | as   |      |
|      |      | radi |      |
|      |      | us   |      |
|      |      | of   |      |
|      |      | glas |      |
|      |      | s    |      |
|      |      | sphe |      |
|      |      | re,  |      |
|      |      | not  |      |
|      |      | the  |      |
|      |      | box! |      |
|      |      | boun |      |
|      |      | ded\ |      |
|      |      | _by  |      |
|      |      | {obj |      |
|      |      | ect  |      |
|      |      | {Uni |      |
|      |      | tBox |      |
|      |      | scal |      |
|      |      | e    |      |
|      |      | 1.75 |      |
|      |      | }}}r |      |
|      |      | otat |      |
|      |      | e    |      |
|      |      | y*\  |      |
|      |      | 45}p |      |
|      |      | lane |      |
|      |      | { y, |      |
|      |      | -1.5 |      |
|      |      | text |      |
|      |      | ure  |      |
|      |      | {T\_ |      |
|      |      | Ston |      |
|      |      | e1pi |      |
|      |      | gmen |      |
|      |      | t    |      |
|      |      | {oct |      |
|      |      | aves |      |
|      |      | 3rot |      |
|      |      | ate  |      |
|      |      | 90\* |      |
|      |      | z}fi |      |
|      |      | nish |      |
|      |      | {    |      |
|      |      | refl |      |
|      |      | ecti |      |
|      |      | on   |      |
|      |      | 0.10 |      |
|      |      | }}}  |      |
+------+------+------+------+
| Widt | widt | 0    | Set  |
| h    | h    |      | canv |
|      |      |      | as   |
|      |      |      | widt |
|      |      |      | h,   |
|      |      |      | defa |
|      |      |      | ult  |
|      |      |      | (0)  |
|      |      |      | is   |
|      |      |      | proj |
|      |      |      | ect  |
|      |      |      | form |
|      |      |      | at   |
+------+------+------+------+
| Heig | heig | 0    | Set  |
| ht   | ht   |      | canv |
|      |      |      | as   |
|      |      |      | heig |
|      |      |      | ht,  |
|      |      |      | defa |
|      |      |      | ult  |
|      |      |      | (0)  |
|      |      |      | is   |
|      |      |      | proj |
|      |      |      | ect  |
|      |      |      | form |
|      |      |      | at   |
+------+------+------+------+
| Qual | qual | 9    | Set  |
| ity  | ity  |      | rend |
|      |      |      | er   |
|      |      |      | qual |
|      |      |      | ity  |
+------+------+------+------+
| Anti | anti | 0    | Set  |
| alia | alia |      | rend |
| sing | sing |      | er   |
|      |      |      | anti |
|      |      |      | alia |
|      |      |      | sing |
+------+------+------+------+
| Alph | alph | On   | Use  |
| a    | a    |      | alph |
|      |      |      | a    |
|      |      |      | chan |
|      |      |      | nel  |
|      |      |      | for  |
|      |      |      | tran |
|      |      |      | spar |
|      |      |      | ency |
|      |      |      | mask |
+------+------+------+------+
| Exec | povP | N/A  | Path |
| utab | ath  |      | to   |
| le   |      |      | POV- |
|      |      |      | Ray  |
|      |      |      | exec |
|      |      |      | utab |
|      |      |      | le   |
+------+------+------+------+
| Incl | povI | N/A  | Path |
| udes | nc   |      | to   |
|      |      |      | POV- |
|      |      |      | Ray  |
|      |      |      | incl |
|      |      |      | udes |
+------+------+------+------+
| Comm | povC | N/A  | Add  |
| andl | md   |      | addi |
| ine  |      |      | tion |
|      |      |      | al   |
|      |      |      | POV- |
|      |      |      | Ray  |
|      |      |      | comm |
|      |      |      | ands |
|      |      |      | here |
|      |      |      | ,    |
|      |      |      | be   |
|      |      |      | care |
|      |      |      | ful! |
+------+------+------+------+
| Star | star | 0    | Set  |
| t    | tFra |      | star |
| Fram | me   |      | t    |
| e    |      |      | fram |
|      |      |      | e    |
+------+------+------+------+
| End  | endF | 0    | Set  |
| Fram | rame |      | end  |
| e    |      |      | fram |
|      |      |      | e    |
+------+------+------+------+
| Node | Node |      |      |
+------+------+------+------+
| Labe | user | N/A  | This |
| l    | Text |      | labe |
|      | Area |      | l    |
|      |      |      | gets |
|      |      |      | appe |
|      |      |      | nded |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | name |
|      |      |      | on   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | grap |
|      |      |      | h.   |
+------+------+------+------+
| Outp | chan | Colo | Sele |
| ut   | nels | r.RG | ct   |
| Laye |      | BA   | here |
| r    |      |      | the  |
|      |      |      | laye |
|      |      |      | r    |
|      |      |      | onto |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | the  |
|      |      |      | proc |
|      |      |      | essi |
|      |      |      | ng   |
|      |      |      | shou |
|      |      |      | ld   |
|      |      |      | occu |
|      |      |      | r.   |
+------+------+------+------+
| R    | Natr | On   | Proc |
|      | onOf |      | ess  |
|      | xPar |      | red  |
|      | amPr |      | comp |
|      | oces |      | onen |
|      | sR   |      | t.   |
+------+------+------+------+
| G    | Natr | On   | Proc |
|      | onOf |      | ess  |
|      | xPar |      | gree |
|      | amPr |      | n    |
|      | oces |      | comp |
|      | sG   |      | onen |
|      |      |      | t.   |
+------+------+------+------+
| B    | Natr | On   | Proc |
|      | onOf |      | ess  |
|      | xPar |      | blue |
|      | amPr |      | comp |
|      | oces |      | onen |
|      | sB   |      | t.   |
+------+------+------+------+
| A    | Natr | On   | Proc |
|      | onOf |      | ess  |
|      | xPar |      | alph |
|      | amPr |      | a    |
|      | oces |      | comp |
|      | sA   |      | onen |
|      |      |      | t.   |
+------+------+------+------+
|      | adva | N/A  |      |
|      | nced |      |      |
|      | Sep  |      |      |
+------+------+------+------+
| Hide | hide | Off  | When |
| inpu | Inpu |      | chec |
| ts   | ts   |      | ked, |
|      |      |      | the  |
|      |      |      | inpu |
|      |      |      | t    |
|      |      |      | arro |
|      |      |      | ws   |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | grap |
|      |      |      | h    |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | hidd |
|      |      |      | en   |
+------+------+------+------+
| Forc | forc | Off  | When |
| e    | eCac |      | chec |
| cach | hing |      | ked, |
| ing  |      |      | the  |
|      |      |      | outp |
|      |      |      | ut   |
|      |      |      | of   |
|      |      |      | this |
|      |      |      | node |
|      |      |      | will |
|      |      |      | alwa |
|      |      |      | ys   |
|      |      |      | be   |
|      |      |      | kept |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | RAM  |
|      |      |      | cach |
|      |      |      | e    |
|      |      |      | for  |
|      |      |      | fast |
|      |      |      | acce |
|      |      |      | ss   |
|      |      |      | of   |
|      |      |      | alre |
|      |      |      | ady  |
|      |      |      | comp |
|      |      |      | uted |
|      |      |      | imag |
|      |      |      | es.  |
+------+------+------+------+
| Prev | enab | Off  | Whet |
| iew  | lePr |      | her  |
|      | evie |      | to   |
|      | w    |      | show |
|      |      |      | a    |
|      |      |      | prev |
|      |      |      | iew  |
|      |      |      | on   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | box  |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | -gra |
|      |      |      | ph.  |
+------+------+------+------+
| Disa | disa | Off  | When |
| ble  | bleN |      | disa |
|      | ode  |      | bled |
|      |      |      | ,    |
|      |      |      | this |
|      |      |      | node |
|      |      |      | acts |
|      |      |      | as a |
|      |      |      | pass |
|      |      |      | thro |
|      |      |      | ugh. |
+------+------+------+------+
| Life | node | x: 0 | This |
| time | Life | y: 0 | is   |
| Rang | Time |      | the  |
| e    |      |      | fram |
|      |      |      | e    |
|      |      |      | rang |
|      |      |      | e    |
|      |      |      | duri |
|      |      |      | ng   |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | acti |
|      |      |      | ve   |
|      |      |      | if   |
|      |      |      | Enab |
|      |      |      | le   |
|      |      |      | Life |
|      |      |      | time |
|      |      |      | is   |
|      |      |      | chec |
|      |      |      | ked  |
+------+------+------+------+
| Enab | enab | Off  | When |
| le   | leNo |      | chec |
| Life | deLi |      | ked, |
| time | feTi |      | the  |
|      | me   |      | node |
|      |      |      | is   |
|      |      |      | only |
|      |      |      | acti |
|      |      |      | ve   |
|      |      |      | duri |
|      |      |      | ng   |
|      |      |      | the  |
|      |      |      | spec |
|      |      |      | ifie |
|      |      |      | d    |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | rang |
|      |      |      | e    |
|      |      |      | by   |
|      |      |      | the  |
|      |      |      | Life |
|      |      |      | time |
|      |      |      | Rang |
|      |      |      | e    |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r.   |
|      |      |      | Outs |
|      |      |      | ide  |
|      |      |      | of   |
|      |      |      | this |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | rang |
|      |      |      | e,   |
|      |      |      | it   |
|      |      |      | beha |
|      |      |      | ves  |
|      |      |      | as   |
|      |      |      | if   |
|      |      |      | the  |
|      |      |      | Disa |
|      |      |      | ble  |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | is   |
|      |      |      | chec |
|      |      |      | ked  |
+------+------+------+------+
| Afte | onPa | N/A  | Set  |
| r    | ramC |      | here |
| para | hang |      | the  |
| m    | ed   |      | name |
| chan |      |      | of a |
| ged  |      |      | func |
| call |      |      | tion |
| back |      |      | defi |
|      |      |      | ned  |
|      |      |      | in   |
|      |      |      | Pyth |
|      |      |      | on   |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | call |
|      |      |      | ed   |
|      |      |      | for  |
|      |      |      | each |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | chan |
|      |      |      | ge.  |
|      |      |      | Eith |
|      |      |      | er   |
|      |      |      | defi |
|      |      |      | ne   |
|      |      |      | this |
|      |      |      | func |
|      |      |      | tion |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | Scri |
|      |      |      | pt   |
|      |      |      | Edit |
|      |      |      | or   |
|      |      |      | or   |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | init |
|      |      |      | .py  |
|      |      |      | scri |
|      |      |      | pt   |
|      |      |      | or   |
|      |      |      | even |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | scri |
|      |      |      | pt   |
|      |      |      | of a |
|      |      |      | Pyth |
|      |      |      | on   |
|      |      |      | grou |
|      |      |      | p    |
|      |      |      | plug |
|      |      |      | -in. |
|      |      |      | The  |
|      |      |      | sign |
|      |      |      | atur |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | call |
|      |      |      | back |
|      |      |      | is:  |
|      |      |      | call |
|      |      |      | back |
|      |      |      | (thi |
|      |      |      | sPar |
|      |      |      | am,  |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | ,    |
|      |      |      | this |
|      |      |      | Grou |
|      |      |      | p,   |
|      |      |      | app, |
|      |      |      | user |
|      |      |      | Edit |
|      |      |      | ed)  |
|      |      |      | wher |
|      |      |      | e:-  |
|      |      |      | this |
|      |      |      | Para |
|      |      |      | m:   |
|      |      |      | The  |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | just |
|      |      |      | had  |
|      |      |      | its  |
|      |      |      | valu |
|      |      |      | e    |
|      |      |      | chan |
|      |      |      | ged- |
|      |      |      | user |
|      |      |      | Edit |
|      |      |      | ed:  |
|      |      |      | A    |
|      |      |      | bool |
|      |      |      | ean  |
|      |      |      | info |
|      |      |      | rmin |
|      |      |      | g    |
|      |      |      | whet |
|      |      |      | her  |
|      |      |      | the  |
|      |      |      | chan |
|      |      |      | ge   |
|      |      |      | was  |
|      |      |      | due  |
|      |      |      | to   |
|      |      |      | user |
|      |      |      | inte |
|      |      |      | ract |
|      |      |      | ion  |
|      |      |      | or   |
|      |      |      | beca |
|      |      |      | use  |
|      |      |      | some |
|      |      |      | thin |
|      |      |      | g    |
|      |      |      | inte |
|      |      |      | rnal |
|      |      |      | ly   |
|      |      |      | trig |
|      |      |      | gere |
|      |      |      | d    |
|      |      |      | the  |
|      |      |      | chan |
|      |      |      | ge.- |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | :    |
|      |      |      | The  |
|      |      |      | node |
|      |      |      | hold |
|      |      |      | ing  |
|      |      |      | the  |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r-   |
|      |      |      | app: |
|      |      |      | poin |
|      |      |      | ts   |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | curr |
|      |      |      | ent  |
|      |      |      | appl |
|      |      |      | icat |
|      |      |      | ion  |
|      |      |      | inst |
|      |      |      | ance |
|      |      |      | -    |
|      |      |      | this |
|      |      |      | Grou |
|      |      |      | p:   |
|      |      |      | The  |
|      |      |      | grou |
|      |      |      | p    |
|      |      |      | hold |
|      |      |      | ing  |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | (onl |
|      |      |      | y    |
|      |      |      | if   |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | belo |
|      |      |      | ngs  |
|      |      |      | to a |
|      |      |      | grou |
|      |      |      | p)   |
+------+------+------+------+
| Afte | onIn | N/A  | Set  |
| r    | putC |      | here |
| inpu | hang |      | the  |
| t    | ed   |      | name |
| chan |      |      | of a |
| ged  |      |      | func |
| call |      |      | tion |
| back |      |      | defi |
|      |      |      | ned  |
|      |      |      | in   |
|      |      |      | Pyth |
|      |      |      | on   |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | call |
|      |      |      | ed   |
|      |      |      | afte |
|      |      |      | r    |
|      |      |      | each |
|      |      |      | conn |
|      |      |      | ecti |
|      |      |      | on   |
|      |      |      | is   |
|      |      |      | chan |
|      |      |      | ged  |
|      |      |      | for  |
|      |      |      | the  |
|      |      |      | inpu |
|      |      |      | ts   |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | .    |
|      |      |      | Eith |
|      |      |      | er   |
|      |      |      | defi |
|      |      |      | ne   |
|      |      |      | this |
|      |      |      | func |
|      |      |      | tion |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | Scri |
|      |      |      | pt   |
|      |      |      | Edit |
|      |      |      | or   |
|      |      |      | or   |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | init |
|      |      |      | .py  |
|      |      |      | scri |
|      |      |      | pt   |
|      |      |      | or   |
|      |      |      | even |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | scri |
|      |      |      | pt   |
|      |      |      | of a |
|      |      |      | Pyth |
|      |      |      | on   |
|      |      |      | grou |
|      |      |      | p    |
|      |      |      | plug |
|      |      |      | -in. |
|      |      |      | The  |
|      |      |      | sign |
|      |      |      | atur |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | call |
|      |      |      | back |
|      |      |      | is:  |
|      |      |      | call |
|      |      |      | back |
|      |      |      | (inp |
|      |      |      | utIn |
|      |      |      | dex, |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | ,    |
|      |      |      | this |
|      |      |      | Grou |
|      |      |      | p,   |
|      |      |      | app) |
|      |      |      | :-   |
|      |      |      | inpu |
|      |      |      | tInd |
|      |      |      | ex:  |
|      |      |      | the  |
|      |      |      | inde |
|      |      |      | x    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | inpu |
|      |      |      | t    |
|      |      |      | whic |
|      |      |      | h    |
|      |      |      | chan |
|      |      |      | ged, |
|      |      |      | you  |
|      |      |      | can  |
|      |      |      | quer |
|      |      |      | y    |
|      |      |      | the  |
|      |      |      | node |
|      |      |      | conn |
|      |      |      | ecte |
|      |      |      | d    |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | inpu |
|      |      |      | t    |
|      |      |      | by   |
|      |      |      | call |
|      |      |      | ing  |
|      |      |      | the  |
|      |      |      | getI |
|      |      |      | nput |
|      |      |      | (... |
|      |      |      | )    |
|      |      |      | func |
|      |      |      | tion |
|      |      |      | .-   |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | :    |
|      |      |      | The  |
|      |      |      | node |
|      |      |      | hold |
|      |      |      | ing  |
|      |      |      | the  |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r-   |
|      |      |      | app: |
|      |      |      | poin |
|      |      |      | ts   |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | curr |
|      |      |      | ent  |
|      |      |      | appl |
|      |      |      | icat |
|      |      |      | ion  |
|      |      |      | inst |
|      |      |      | ance |
|      |      |      | -    |
|      |      |      | this |
|      |      |      | Grou |
|      |      |      | p:   |
|      |      |      | The  |
|      |      |      | grou |
|      |      |      | p    |
|      |      |      | hold |
|      |      |      | ing  |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | (onl |
|      |      |      | y    |
|      |      |      | if   |
|      |      |      | this |
|      |      |      | Node |
|      |      |      | belo |
|      |      |      | ngs  |
|      |      |      | to a |
|      |      |      | grou |
|      |      |      | p)   |
+------+------+------+------+
| Info | Info |      |      |
+------+------+------+------+
|      | node | N/A  | Inpu |
|      | Info |      | t    |
|      | s    |      | and  |
|      |      |      | outp |
|      |      |      | ut   |
|      |      |      | info |
|      |      |      | rmat |
|      |      |      | ions |
|      |      |      | ,    |
|      |      |      | pres |
|      |      |      | s    |
|      |      |      | Refr |
|      |      |      | esh  |
|      |      |      | to   |
|      |      |      | upda |
|      |      |      | te   |
|      |      |      | them |
|      |      |      | with |
|      |      |      | curr |
|      |      |      | ent  |
|      |      |      | valu |
|      |      |      | es   |
+------+------+------+------+
| Refr | refr | N/A  |      |
| esh  | eshB |      |      |
| Info | utto |      |      |
|      | n    |      |      |
+------+------+------+------+
| Sour |      |      | Sour |
| ce   |      |      | ce   |
+------+------+------+------+
