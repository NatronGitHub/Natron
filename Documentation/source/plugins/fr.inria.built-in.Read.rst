Read
====

*This documentation is for version 1.0 of Read.*

Node used to read images or videos from disk. The image/video is identified by its filename and its extension. Given the extension, the Reader selected from the Preferences to decode that specific format will be used.

Inputs & Controls
-----------------

+------+------+------+------+
| Labe | Scri | Defa | Func |
| l    | pt-N | ult- | tion |
| (UI  | ame  | Valu |      |
| Name |      | e    |      |
| )    |      |      |      |
+======+======+======+======+
| Cont | Cont |      |      |
| rols | rols |      |      |
+------+------+------+------+
| File | file | N/A  | Pres |
| Info | Info |      | s    |
| ...  |      |      | to   |
|      |      |      | disp |
|      |      |      | lay  |
|      |      |      | info |
|      |      |      | rmat |
|      |      |      | ions |
|      |      |      | abou |
|      |      |      | t    |
|      |      |      | the  |
|      |      |      | file |
+------+------+------+------+
| Deco | deco | Defa | Sele |
| der  | ding | ult  | ct   |
|      | Plug |      | the  |
|      | inCh |      | inte |
|      | oice |      | rnal |
|      |      |      | deco |
|      |      |      | der  |
|      |      |      | plug |
|      |      |      | -in  |
|      |      |      | used |
|      |      |      | for  |
|      |      |      | this |
|      |      |      | file |
|      |      |      | form |
|      |      |      | at.  |
|      |      |      | By   |
|      |      |      | defa |
|      |      |      | ult  |
|      |      |      | this |
|      |      |      | uses |
|      |      |      | the  |
|      |      |      | plug |
|      |      |      | -in  |
|      |      |      | sele |
|      |      |      | cted |
|      |      |      | for  |
|      |      |      | this |
|      |      |      | file |
|      |      |      | exte |
|      |      |      | nsio |
|      |      |      | n    |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | Pref |
|      |      |      | eren |
|      |      |      | ces  |
|      |      |      | of   |
|      |      |      | Natr |
|      |      |      | on   |
+------+------+------+------+
| Deco | deco | N/A  | Belo |
| der  | derO |      | w    |
| Opti | ptio |      | can  |
| ons  | nsSe |      | be   |
|      | para |      | foun |
|      | tor  |      | d    |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | rs   |
|      |      |      | that |
|      |      |      | are  |
|      |      |      | spec |
|      |      |      | ific |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | Read |
|      |      |      | er   |
|      |      |      | plug |
|      |      |      | -in. |
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
| Prev | enab | On   | Whet |
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
| File | file | N/A  | The  |
|      | name |      | inpu |
|      |      |      | t    |
|      |      |      | imag |
|      |      |      | e    |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | /vid |
|      |      |      | eo   |
|      |      |      | stre |
|      |      |      | am   |
|      |      |      | file |
|      |      |      | (s). |
+------+------+------+------+
| Firs | firs | 0    | The  |
| t    | tFra |      | firs |
| Fram | me   |      | t    |
| e    |      |      | fram |
|      |      |      | e    |
|      |      |      | this |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | /vid |
|      |      |      | eo   |
|      |      |      | shou |
|      |      |      | ld   |
|      |      |      | star |
|      |      |      | t    |
|      |      |      | at.  |
|      |      |      | This |
|      |      |      | cann |
|      |      |      | ot   |
|      |      |      | be   |
|      |      |      | less |
|      |      |      | than |
|      |      |      | the  |
|      |      |      | firs |
|      |      |      | t    |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | and  |
|      |      |      | cann |
|      |      |      | ot   |
|      |      |      | be   |
|      |      |      | grea |
|      |      |      | ter  |
|      |      |      | than |
|      |      |      | the  |
|      |      |      | last |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | .    |
+------+------+------+------+
| Befo | befo | Hold | What |
| re   | re   |      | to   |
|      |      |      | do   |
|      |      |      | befo |
|      |      |      | re   |
|      |      |      | the  |
|      |      |      | firs |
|      |      |      | t    |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | .    |
+------+------+------+------+
| Last | last | 0    | The  |
| Fram | Fram |      | fram |
| e    | e    |      | e    |
|      |      |      | this |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | /vid |
|      |      |      | eo   |
|      |      |      | shou |
|      |      |      | ld   |
|      |      |      | end  |
|      |      |      | at.  |
|      |      |      | This |
|      |      |      | cann |
|      |      |      | ot   |
|      |      |      | be   |
|      |      |      | less |
|      |      |      | er   |
|      |      |      | than |
|      |      |      | the  |
|      |      |      | firs |
|      |      |      | t    |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | and  |
|      |      |      | cann |
|      |      |      | ot   |
|      |      |      | be   |
|      |      |      | grea |
|      |      |      | ter  |
|      |      |      | than |
|      |      |      | the  |
|      |      |      | last |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | .    |
+------+------+------+------+
| Afte | afte | Hold | What |
| r    | r    |      | to   |
|      |      |      | do   |
|      |      |      | afte |
|      |      |      | r    |
|      |      |      | the  |
|      |      |      | last |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | .    |
+------+------+------+------+
| On   | onMi | Erro | What |
| Miss | ssin | r    | to   |
| ing  | gFra |      | do   |
| Fram | me   |      | when |
| e    |      |      | a    |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | is   |
|      |      |      | miss |
|      |      |      | ing  |
|      |      |      | from |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | /str |
|      |      |      | eam. |
+------+------+------+------+
| Fram | fram | Star |      |
| e    | eMod | ting |      |
| Mode | e    | Time |      |
+------+------+------+------+
| Star | star | 0    | At   |
| ting | ting |      | what |
| Time | Time |      | time |
|      |      |      | (on  |
|      |      |      | the  |
|      |      |      | time |
|      |      |      | line |
|      |      |      | )    |
|      |      |      | shou |
|      |      |      | ld   |
|      |      |      | this |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | /vid |
|      |      |      | eo   |
|      |      |      | star |
|      |      |      | t.   |
+------+------+------+------+
| Time | time | 0    | Offs |
| Offs | Offs |      | et   |
| et   | et   |      | appl |
|      |      |      | ied  |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | sequ |
|      |      |      | ence |
|      |      |      | in   |
|      |      |      | time |
|      |      |      | unit |
|      |      |      | s    |
|      |      |      | (i.e |
|      |      |      | .    |
|      |      |      | fram |
|      |      |      | es). |
+------+------+------+------+
| Prox | prox | N/A  | File |
| y    | y    |      | name |
| File |      |      | of   |
|      |      |      | the  |
|      |      |      | prox |
|      |      |      | y    |
|      |      |      | imag |
|      |      |      | es.  |
|      |      |      | They |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | used |
|      |      |      | inst |
|      |      |      | ead  |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | imag |
|      |      |      | es   |
|      |      |      | read |
|      |      |      | from |
|      |      |      | the  |
|      |      |      | File |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | when |
|      |      |      | the  |
|      |      |      | prox |
|      |      |      | y    |
|      |      |      | mode |
|      |      |      | (dow |
|      |      |      | nsca |
|      |      |      | ling |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | imag |
|      |      |      | es)  |
|      |      |      | is   |
|      |      |      | acti |
|      |      |      | vate |
|      |      |      | d.   |
+------+------+------+------+
| Prox | prox | x: 1 | The  |
| y    | yThr | y: 1 | orig |
| thre | esho |      | inal |
| shol | ld   |      | scal |
| d    |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | prox |
|      |      |      | y    |
|      |      |      | imag |
|      |      |      | e.   |
+------+------+------+------+
| Cust | cust | Off  | Chec |
| om   | omPr |      | k    |
| Prox | oxyS |      | to   |
| y    | cale |      | enab |
| Scal |      |      | le   |
| e    |      |      | the  |
|      |      |      | Prox |
|      |      |      | y    |
|      |      |      | scal |
|      |      |      | e    |
|      |      |      | edit |
|      |      |      | ion. |
+------+------+------+------+
| File | file | PreM | The  |
| Prem | Prem | ulti | imag |
| ult  | ult  | plie | e    |
|      |      | d    | file |
|      |      |      | bein |
|      |      |      | g    |
|      |      |      | read |
|      |      |      | is   |
|      |      |      | cons |
|      |      |      | ider |
|      |      |      | ed   |
|      |      |      | to   |
|      |      |      | have |
|      |      |      | this |
|      |      |      | prem |
|      |      |      | ulti |
|      |      |      | plic |
|      |      |      | atio |
|      |      |      | n    |
|      |      |      | stat |
|      |      |      | e.On |
|      |      |      | outp |
|      |      |      | ut,  |
|      |      |      | RGB  |
|      |      |      | imag |
|      |      |      | es   |
|      |      |      | are  |
|      |      |      | alwa |
|      |      |      | ys   |
|      |      |      | Opaq |
|      |      |      | ue,  |
|      |      |      | Alph |
|      |      |      | a    |
|      |      |      | and  |
|      |      |      | RGBA |
|      |      |      | imag |
|      |      |      | es   |
|      |      |      | are  |
|      |      |      | alwa |
|      |      |      | ys   |
|      |      |      | Prem |
|      |      |      | ulti |
|      |      |      | plie |
|      |      |      | d    |
|      |      |      | (als |
|      |      |      | o    |
|      |      |      | call |
|      |      |      | ed   |
|      |      |      | "ass |
|      |      |      | ocia |
|      |      |      | ted  |
|      |      |      | alph |
|      |      |      | a"). |
|      |      |      | To   |
|      |      |      | get  |
|      |      |      | UnPr |
|      |      |      | emul |
|      |      |      | tipl |
|      |      |      | ied  |
|      |      |      | (or  |
|      |      |      | "una |
|      |      |      | ssoc |
|      |      |      | iate |
|      |      |      | d    |
|      |      |      | alph |
|      |      |      | a")  |
|      |      |      | imag |
|      |      |      | es,  |
|      |      |      | use  |
|      |      |      | the  |
|      |      |      | "Unp |
|      |      |      | remu |
|      |      |      | lt"  |
|      |      |      | plug |
|      |      |      | in   |
|      |      |      | afte |
|      |      |      | r    |
|      |      |      | this |
|      |      |      | plug |
|      |      |      | in.- |
|      |      |      | Opaq |
|      |      |      | ue   |
|      |      |      | mean |
|      |      |      | s    |
|      |      |      | that |
|      |      |      | the  |
|      |      |      | alph |
|      |      |      | a    |
|      |      |      | chan |
|      |      |      | nel  |
|      |      |      | is   |
|      |      |      | cons |
|      |      |      | ider |
|      |      |      | ed   |
|      |      |      | to   |
|      |      |      | be 1 |
|      |      |      | (one |
|      |      |      | ),   |
|      |      |      | and  |
|      |      |      | it   |
|      |      |      | is   |
|      |      |      | not  |
|      |      |      | take |
|      |      |      | n    |
|      |      |      | into |
|      |      |      | acco |
|      |      |      | unt  |
|      |      |      | in   |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce   |
|      |      |      | conv |
|      |      |      | ersi |
|      |      |      | on.- |
|      |      |      | Prem |
|      |      |      | ulti |
|      |      |      | plie |
|      |      |      | d,   |
|      |      |      | red, |
|      |      |      | gree |
|      |      |      | n    |
|      |      |      | and  |
|      |      |      | blue |
|      |      |      | chan |
|      |      |      | nels |
|      |      |      | are  |
|      |      |      | divi |
|      |      |      | ded  |
|      |      |      | by   |
|      |      |      | the  |
|      |      |      | alph |
|      |      |      | a    |
|      |      |      | chan |
|      |      |      | nel  |
|      |      |      | befo |
|      |      |      | re   |
|      |      |      | appl |
|      |      |      | ying |
|      |      |      | the  |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce   |
|      |      |      | conv |
|      |      |      | ersi |
|      |      |      | on,  |
|      |      |      | and  |
|      |      |      | re-m |
|      |      |      | ulti |
|      |      |      | plie |
|      |      |      | d    |
|      |      |      | by   |
|      |      |      | alph |
|      |      |      | a    |
|      |      |      | afte |
|      |      |      | r    |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce   |
|      |      |      | conv |
|      |      |      | ersi |
|      |      |      | on.- |
|      |      |      | UnPr |
|      |      |      | emul |
|      |      |      | tipl |
|      |      |      | ied, |
|      |      |      | mean |
|      |      |      | s    |
|      |      |      | that |
|      |      |      | red, |
|      |      |      | gree |
|      |      |      | n    |
|      |      |      | and  |
|      |      |      | blue |
|      |      |      | chan |
|      |      |      | nels |
|      |      |      | are  |
|      |      |      | not  |
|      |      |      | modi |
|      |      |      | fied |
|      |      |      | befo |
|      |      |      | re   |
|      |      |      | appl |
|      |      |      | ying |
|      |      |      | the  |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce   |
|      |      |      | conv |
|      |      |      | ersi |
|      |      |      | on,  |
|      |      |      | and  |
|      |      |      | are  |
|      |      |      | mult |
|      |      |      | ipli |
|      |      |      | ed   |
|      |      |      | by   |
|      |      |      | alph |
|      |      |      | a    |
|      |      |      | afte |
|      |      |      | r    |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce   |
|      |      |      | conv |
|      |      |      | ersi |
|      |      |      | on.T |
|      |      |      | his  |
|      |      |      | is   |
|      |      |      | set  |
|      |      |      | auto |
|      |      |      | mati |
|      |      |      | call |
|      |      |      | y    |
|      |      |      | from |
|      |      |      | the  |
|      |      |      | imag |
|      |      |      | e    |
|      |      |      | file |
|      |      |      | and  |
|      |      |      | the  |
|      |      |      | plug |
|      |      |      | in,  |
|      |      |      | but  |
|      |      |      | can  |
|      |      |      | be   |
|      |      |      | adju |
|      |      |      | sted |
|      |      |      | if   |
|      |      |      | this |
|      |      |      | info |
|      |      |      | rmat |
|      |      |      | ion  |
|      |      |      | is   |
|      |      |      | wron |
|      |      |      | g    |
|      |      |      | in   |
|      |      |      | the  |
|      |      |      | file |
|      |      |      | meta |
|      |      |      | data |
|      |      |      | .RGB |
|      |      |      | imag |
|      |      |      | es   |
|      |      |      | can  |
|      |      |      | only |
|      |      |      | be   |
|      |      |      | Opaq |
|      |      |      | ue,  |
|      |      |      | and  |
|      |      |      | Alph |
|      |      |      | a    |
|      |      |      | imag |
|      |      |      | es   |
|      |      |      | can  |
|      |      |      | only |
|      |      |      | be   |
|      |      |      | Prem |
|      |      |      | ulti |
|      |      |      | plie |
|      |      |      | d    |
|      |      |      | (the |
|      |      |      | valu |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | this |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | does |
|      |      |      | n't  |
|      |      |      | matt |
|      |      |      | er). |
+------+------+------+------+
| Outp | outp | RGBA | What |
| ut   | utCo |      | type |
| Comp | mpon |      | of   |
| onen | ents |      | comp |
| ts   |      |      | onen |
|      |      |      | ts   |
|      |      |      | this |
|      |      |      | effe |
|      |      |      | ct   |
|      |      |      | shou |
|      |      |      | ld   |
|      |      |      | outp |
|      |      |      | ut   |
|      |      |      | when |
|      |      |      | the  |
|      |      |      | main |
|      |      |      | colo |
|      |      |      | r    |
|      |      |      | plan |
|      |      |      | e    |
|      |      |      | is   |
|      |      |      | requ |
|      |      |      | este |
|      |      |      | d.   |
|      |      |      | For  |
|      |      |      | the  |
|      |      |      | Read |
|      |      |      | node |
|      |      |      | it   |
|      |      |      | will |
|      |      |      | map  |
|      |      |      | (in  |
|      |      |      | numb |
|      |      |      | er   |
|      |      |      | of   |
|      |      |      | comp |
|      |      |      | onen |
|      |      |      | ts)  |
|      |      |      | the  |
|      |      |      | Outp |
|      |      |      | ut   |
|      |      |      | Laye |
|      |      |      | r    |
|      |      |      | choi |
|      |      |      | ce   |
|      |      |      | to   |
|      |      |      | thes |
|      |      |      | e.   |
+------+------+------+------+
| Fram | fram | 24   | By   |
| e    | eRat |      | defa |
| rate | e    |      | ult  |
|      |      |      | this |
|      |      |      | valu |
|      |      |      | e    |
|      |      |      | is   |
|      |      |      | gues |
|      |      |      | sed  |
|      |      |      | from |
|      |      |      | the  |
|      |      |      | file |
|      |      |      | .    |
|      |      |      | You  |
|      |      |      | can  |
|      |      |      | over |
|      |      |      | ride |
|      |      |      | it   |
|      |      |      | by   |
|      |      |      | chec |
|      |      |      | king |
|      |      |      | the  |
|      |      |      | Cust |
|      |      |      | om   |
|      |      |      | fps  |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r.   |
|      |      |      | The  |
|      |      |      | valu |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | this |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r    |
|      |      |      | is   |
|      |      |      | what |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | visi |
|      |      |      | ble  |
|      |      |      | by   |
|      |      |      | the  |
|      |      |      | effe |
|      |      |      | cts  |
|      |      |      | down |
|      |      |      | -str |
|      |      |      | eam. |
+------+------+------+------+
| Cust | cust | Off  | If   |
| om   | omFp |      | chec |
| FPS  | s    |      | ked, |
|      |      |      | you  |
|      |      |      | can  |
|      |      |      | free |
|      |      |      | ly   |
|      |      |      | forc |
|      |      |      | e    |
|      |      |      | the  |
|      |      |      | valu |
|      |      |      | e    |
|      |      |      | of   |
|      |      |      | the  |
|      |      |      | fram |
|      |      |      | e    |
|      |      |      | rate |
|      |      |      | para |
|      |      |      | mete |
|      |      |      | r.   |
|      |      |      | The  |
|      |      |      | fram |
|      |      |      | e-ra |
|      |      |      | te   |
|      |      |      | is   |
|      |      |      | just |
|      |      |      | the  |
|      |      |      | meta |
|      |      |      | -dat |
|      |      |      | a    |
|      |      |      | that |
|      |      |      | will |
|      |      |      | be   |
|      |      |      | pass |
|      |      |      | ed   |
|      |      |      | down |
|      |      |      | stre |
|      |      |      | am   |
|      |      |      | to   |
|      |      |      | the  |
|      |      |      | grap |
|      |      |      | h,   |
|      |      |      | no   |
|      |      |      | reti |
|      |      |      | me   |
|      |      |      | will |
|      |      |      | actu |
|      |      |      | ally |
|      |      |      | take |
|      |      |      | plac |
|      |      |      | e.   |
+------+------+------+------+
| OCIO | ocio | [OCI | Open |
| Conf | Conf | O]/c | Colo |
| ig   | igFi | onfi | rIO  |
| File | le   | g.oc | conf |
|      |      | io   | igur |
|      |      |      | atio |
|      |      |      | n    |
|      |      |      | file |
+------+------+------+------+
| File | ocio | scen | Inpu |
| Colo | Inpu | e\_l | t    |
| rspa | tSpa | inea | data |
| ce   | ce   | r    | is   |
|      |      |      | take |
|      |      |      | n    |
|      |      |      | to   |
|      |      |      | be   |
|      |      |      | in   |
|      |      |      | this |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce.  |
+------+------+------+------+
| File | ocio | aces | Inpu |
| Colo | Inpu | /Lin | t    |
| rspa | tSpa | ear  | data |
| ce   | ceIn |      | is   |
|      | dex  |      | take |
|      |      |      | n    |
|      |      |      | to   |
|      |      |      | be   |
|      |      |      | in   |
|      |      |      | this |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce.  |
+------+------+------+------+
| Outp | ocio | scen | Outp |
| ut   | Outp | e\_l | ut   |
| Colo | utSp | inea | data |
| rspa | ace  | r    | is   |
| ce   |      |      | take |
|      |      |      | n    |
|      |      |      | to   |
|      |      |      | be   |
|      |      |      | in   |
|      |      |      | this |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce.  |
+------+------+------+------+
| Outp | ocio | aces | Outp |
| ut   | Outp | /Lin | ut   |
| Colo | utSp | ear  | data |
| rspa | aceI |      | is   |
| ce   | ndex |      | take |
|      |      |      | n    |
|      |      |      | to   |
|      |      |      | be   |
|      |      |      | in   |
|      |      |      | this |
|      |      |      | colo |
|      |      |      | rspa |
|      |      |      | ce.  |
+------+------+------+------+
| OCIO | ocio | N/A  | Help |
| conf | Help |      | abou |
| ig   |      |      | t    |
| help |      |      | the  |
| ...  |      |      | Open |
|      |      |      | Colo |
|      |      |      | rIO  |
|      |      |      | conf |
|      |      |      | igur |
|      |      |      | atio |
|      |      |      | n.   |
+------+------+------+------+
|      |      |      | Sync |
+------+------+------+------+
