//
//   Copyright 2009 Free Software Foundation, Inc.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Remember we can't use "pop" or "push" because Gatesware doesn't
// recognise them.

//************************************************************************
//************************************************************************
//		Location of self
//		****************
//************************************************************************
//************************************************************************

Self_location = "/gbjsfunc.js";

//************************************************************************
//************************************************************************

//========================================================================
//
//  Standard stuff to refresh window on completion

var Compl_win, IntervalID;

function check_compl()  {
    if  (top.List.Compl_win.closed)  {
	top.List.window.clearInterval(top.List.IntervalID);
	top.List.document.location.reload();
    }
}

function compl_goback()  {
    if  (top.List.Compl_win.closed)  {
	top.List.window.clearInterval(top.List.IntervalID);
	top.List.document.location = urlnoarg(top.List.document.location);
    }
}

function timer_setup(w, fun, cchk)  {
    top.List.Compl_win = w;
    top.List.IntervalID = top.List.window.setInterval(fun? "compl_goback()": "check_compl()", cchk);
}

//========================================================================
//
//  Handy arithmetic functions
//
//------------------------------------------------------------------------

function rounddown(n, tobase)
{
    return  Math.floor(n / tobase) * tobase;
}

function roundup(n, tobase)
{
    return  Math.ceil(n / tobase) * tobase;
}

function modulo(n, base)
{
    return  n - rounddown(n, base);
}

function roundtomidday(dat) {
    return  rounddown(dat, 24 * 3600000) + 12 * 3600000;
}

//========================================================================
//
// Functions to manipulate URLs
//
//------------------------------------------------------------------------

function urldir(l)  {
    var hr = l.href;
    var ind = hr.lastIndexOf('/');
    return  ind >= 0? hr.substr(0, ind+1): hr;
}

function urlnoarg(l) {
    var hr = l.href, sr = l.search, ind;
    if  (sr != ""  &&  (ind = hr.lastIndexOf(sr)) >= 0)
	hr = hr.substr(0, ind);
    return  hr;
}

function urlandfirstarg(l)  {
    var hr = l.href;
    var ind = hr.indexOf('&');
    return  ind > 0?  hr.substr(0, ind):  hr;
}

function basename(pth)  {
    var broken = pth.split("/");
    return  broken[broken.length-1];
}

function nosuffix(fn)  {
    return  fn.replace(/\..*$/, "");
}

function is_remote(hr)  {
    var ind = hr.lastIndexOf('/');
    var st = ind >= 0? hr.substr(ind+1, 1): hr;
    return  st == "r";
}

function remote_vn(loc, prog)  {
    return is_remote(loc.href)? "r" + prog: prog;
}

function doc_makenewurl(doc, prog)
{
    var loc = doc.location;
    var result = urldir(loc) + remote_vn(loc, prog);
    var sstr = loc.search.substr(1);
    var aind = sstr.indexOf('&');
    if  (aind > 0)
	sstr = sstr.substr(0, aind);
    return  result + '?' + sstr;
}

function makenewurl(prog)
{
    return  doc_makenewurl(top.List.document, prog);
}

function esc_itemlist(itemlist)  {
    var  escitemlist = new Array();
    for  (var cnt = 0;  cnt < itemlist.length;  cnt++)
	escitemlist = escitemlist.concat(escape(itemlist[cnt]));
    return  '&' + escitemlist.join('&');
}

function removechars(str, ch)
{
    var  ind;

    while  ((ind = str.indexOf(ch)) >= 0)
	str = str.substr(0, ind) + str.substr(ind+1);
    return  str;
}

// De-htmlify text
function fixdescr(descr)  {
   return   removechars(removechars(descr, '<'), '>');
}

//========================================================================
//
//	Form creation routines
//
//========================================================================

function text_box(doc, name, sz, maxl)  {
    doc.writeln("<input type=text name=", name, " size=", sz, " maxlength=", maxl, ">");
}

function text_box_init(doc, name, sz, maxl, initv)  {
    doc.writeln("<input type=text name=", name, " size=", sz, " maxlength=", maxl, " value=\"", initv, "\">");
}

function checkbox_item(doc, name, descr)  {
    doc.writeln("<input type=checkbox name=", name, " value=1>", descr);
}

function checkbox_item_chk(doc, name, descr, chk)  {
    doc.write("<input type=checkbox name=", name, " value=1");
    if  (chk)
	doc.write(" checked");
    doc.writeln(">", descr);
}

function radio_button(doc, name, val, descr, isdef)  {
    doc.write("<input type=radio name=", name, " value=");
    if  (isnumeric(val))
	doc.write(val);
    else
	doc.write("\"", val, "\"");
    if  (isdef)
	doc.write(" checked");
    doc.writeln(">", descr);
}

function submit_button(doc, name, buttontxt, clickrout)  {
    doc.writeln("<input type=submit name=", name, " value=\"", buttontxt, "\" onclick=\"", clickrout, ";\">");
}

function action_buttons(doc, actbutttxt)  {
    doc.writeln("<input type=submit name=Action value=\"", actbutttxt, "\">");
    submit_button(doc, "Action", "Cancel", "window.close();return false");
}

function hidden_value(doc, name, value)  {
    doc.writeln("<input type=hidden name=", name, " value=\"", value, "\">");
}

//========================================================================
//
// Get standard bits and pieces from standard places
//
//========================================================================

function get_chgitemlist()  {
    return  esc_itemlist(document.js_func_form.js_itemlist.value.split(','));
}

function get_chgurl()  {
    return  document.js_func_form.js_url.value;
}

//========================================================================
//
// Cookie handling
//
//------------------------------------------------------------------------

function matchcookname(nam, str)
{
    var name = nam + '=', start = 0;
    while ((start = str.indexOf(name, start)) >= 0)  {
	if  (start > 0)  {
	    var  c = str.charAt(start-1);
	    if  ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))  {
		start += name.length;
		continue;
	    }
	}
	return  start + name.length;
    }
    return  -1;
}

function cookval(nam, str)
{
    var ind = matchcookname(nam, str);
    if  (ind < 0)
	return  0;
    var endp = str.indexOf(';', ind);
    return  str.substring(ind, endp < 0? str.length: endp);
}
    
// Get user code and cookies for current program name

function getcookies(pn)  {
    var allcookies = document.cookie;
    var result = new Object();
    if  (allcookies == "")
	return  result;
    var cv = cookval("uc", allcookies);
    if  (cv)
	result.usercode = parseInt(cv);
    cv = cookval(pn, allcookies);
    if  (cv)  {
	var varr = cv.split('&');
	for  (var j = 0;  j < varr.length;  j++)  {
	    var b = varr[j].split(':');
	    result[b[0]] = unescape(b[1]);
	}
    }
    return  result;
}

// Store cookies for program name.

function storecookie(name, val, days)  {
    var expdate = new Date((new Date()).getTime() + days*24*3600000);
    document.cookie = name + '=' + val + '; expires=' + expdate.toGMTString();
}

function storecookies(obj, pn, days) {
    if  (obj.usercode)
	storecookie("uc", obj.usercode, days);
    var carr = new Array();
    for  (var el in obj)  {
	if  (el == "usercode")
	    continue;
	carr = carr.concat(el + ':' + escape(obj[el]));
    }
    if  (carr.length == 0)
	carr = carr.concat("xxx:1");
    storecookie(pn, carr.join('&'), days);
}

//========================================================================
//
// Login type stuff
//
//------------------------------------------------------------------------

// Check for all blanks

function isblank(sv)  {
    var s = sv.value;	
    for (var i = 0;  i < s.length;  i++)  {
	var c = s.charAt(i);
	if  (c != ' '  &&  c != '\t'  &&  c != '\n')
	    return  false;
    }
    return  true;
}

function isnumeric(str)  {
    if  (str.length <= 0)
	return  false;
    for  (var i = 0;  i < str.length;  i++)  {
	var c = str.charAt(i);
	if  (c < '0' || c > '9')
	    return  false;
    }
    return  true;
}

function logincheck(f)  {
    if  (isblank(f.login))  {
	alert("You didn't give a login name");
	f.login.focus();
	return false;
    }
    if  (isblank(f.passwd))  {
	alert("You didn't give a password");
	f.passwd.focus();
	return false;
    }
    return  true;
}

function dologinform(prog)  {
    document.writeln("<SCRIPT SRC=\"", Self_location, "\"></SCRIPT>");
    document.writeln('<FORM NAME="loginform" METHOD=POST onSubmit="return logincheck(document.loginform);" ACTION="',
		     prog, '?login">');
    document.writeln("<PRE>");
    var ind = prog.lastIndexOf('/');
    var st = ind >= 0? prog.substr(ind+1, 1): prog;
    if  (st == "r")
	document.writeln('Host name:  <INPUT TYPE=TEXT NAME=desthost size=20 maxlength=70>');
    document.writeln('Login name: <INPUT TYPE=TEXT NAME=login size=20 maxlength=20>');
    document.writeln('Password:   <INPUT TYPE=PASSWORD NAME=passwd size=20 maxlength=20>');
    document.writeln("</PRE>");
    document.writeln('<INPUT TYPE=SUBMIT NAME="LoginU" VALUE="Login as User">\n</FORM>');
    //    document.loginform.login.focus();
}

function defloginopt(def, prog)  {
    if  (def)  {
	document.write('<br>If you want to log in as the default user ');
	document.writeln('<a href="', prog, '">click here</a>');
    }
}

//  Invoke program after we've got the relevant cookie list

function invokeprog(prog, cooklist)  {
    var aarr = new Array();
    aarr = aarr.concat(cooklist.usercode.toString());
    for (var arg in cooklist)  {
	if  (arg == "usercode" || arg == "xxx")
	    continue;
	aarr = aarr.concat(escape(arg + '=' + cooklist[arg]));
    }
    window.location = prog + '?' + aarr.join('&');
}

// If no login, see if we've got a usercode saved up from the last time.
// If not, do login form.

function ifnologin() {
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    if  (cooklist.usercode)
	invokeprog(prog, cooklist);
    else
	dologinform(prog);
}

function relogin()  {
    dologinform(urlnoarg(top.List.document.location));
    //return  false;
}

function invoke_opt_prog(optname)  {
    var prog = urlnoarg(top.List.document.location);
    var ucode = top.List.document.location.search;
    if  (ucode == "")  {
	alert("No search? Probable bug");
	return  false;
    }
    ucode = parseInt(ucode.substr(1));
    top.List.window.location = prog + '?' + ucode + '&' + optname;
    return  false;
}

//========================================================================
//
// View operations
//
//------------------------------------------------------------------------

function page_viewnext(n)  {
    var targpage = savepage + n;
    if  (targpage < 0)
	targpage = 1;
    else  if  (targpage > savenpages)
	targpage = savenpages;
    var loc = document.location;
    var prog = urlnoarg(loc), sstr = loc.search.substr(1);
    var aind = sstr.indexOf('&');
    if  (aind > 0)
	sstr = sstr.substr(0, aind);
    sstr += '&' + escape(savejobnum) + '&' + targpage;
    document.location = prog + '?' + sstr;
    return  false;
}

function page_viewheader(jn, jt, pg, np, sp, ep, hp, okmod) {
    viewpages = true;
    savejobnum = jn;
    savejobtit = jt;
    savepage = pg;
    savenpages = np;
    savestart = sp;
    saveend = ep;
    savehalt = hp;
    saveokmod = okmod;
    document.write("<table width=\"100%\"><tr><th align=left>");
    if (jt == "")
	document.write("Job number ", jn);
    else
	document.write("<I>", jt, "</I> (", jn, ")");
    document.write("</th><th align=right>Page ", pg, "/", np);
    if (pg == sp)
	document.write("\tStart page");
    if (pg == ep)
	document.write("\tEnd page");
    if (pg == hp)
	document.write("\tPrinting halted here");
    document.writeln("</th></tr></table><hr>");
}

function viewheader(jn, jt, okmod) {
    viewpages = false;
    savejobnum = jn;
    savejobtit = jt;
    saveokmod = okmod;
    document.write("<table width=\"100%\"><tr><th align=left>");
    if (jt == "")
	document.write("Job number ", jn);
    else
	document.write("<I>", jt, "</I> (", jn, ")");
    document.writeln("</th><th align=right>All</th></tr></table><hr>");
}

function set_pageto(argname, page, jn)  {
    document.location = doc_makenewurl(document, "sqccgi") + '&' + escape(argname + '=' + page) + '&' + escape(jn);
    return  false;
}

function view_pageaction(doc, descr, func)  {
    submit_button(doc, "viewaction", descr, func);
}

function view_changepage(doc, descr, func)  {
    view_pageaction(doc, descr, "return " + func);
}

function view_setchangepage(doc, descr, pn, spage)
{
    view_changepage(doc, descr, "set_pageto('" + pn + "', " + spage + ",'" + savejobnum + "')");
}

function viewfooter() {
    document.writeln("<HR>\n<FORM METHOD=GET>");
    if  (viewpages  &&  savepage > 1)  {
	view_changepage(document, "First page", "page_viewnext(-1000000)");
	if  (savepage > 2)
	    view_changepage(document, "Previous page", "page_viewnext(-1)");
    }
    view_pageaction(document, "Quit", "window.close()");
    if  (viewpages)  {
	if  (savepage < savenpages)  {
	    if  (savenpages - savepage > 1)
		view_changepage(document, "Next page", "page_viewnext(1)");
	    view_changepage(document, "Last page", "page_viewnext(1000000)");
	}
	if  (saveokmod)  {
	    if  (savepage != savestart)
		view_setchangepage(document, "Set start", "sp", savepage);
	    if  (savepage != saveend)
		view_setchangepage(document, "Set end", "ep", savepage);
	    if  (savehalt != 0)  {
		if  (savepage != savehalt)
		    view_setchangepage(document, "Set halted at", "hatp", savepage);
		view_setchangepage(document, "Reset halted at", "hatp", 0);
	    }
	}
    }
    document.writeln("</FORM>");
}

function dispvwin(url, jnum, pnum)
{
    var narg = '&' + escape(jnum);
    if  (pnum != "")
	    narg += '&' + escape(pnum);
    url += narg;
    var jnc = jnum, ind = 0;
    for (ind = jnc.indexOf(':', ind);  ind >= 0;  ind = jnc.indexOf(':', ind))
	jnc = jnc.substr(0, ind) + "_" + jnc.substr(ind+1);
    open(url, jnc, "status=yes,resizable=yes,scrollbars=yes");
}

//========================================================================
//
// Utility functions for "main screen"
//
//------------------------------------------------------------------------

function listclicked()  {
    var result = new Array(), els = top.List.document.listform.elements, nam = "";
    for (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "checkbox")  {
	    nam = tag.name;
	    if  (tag.checked)
		result = result.concat(tag);
	}
    }
    if  (result.length == 0)
	alert("No " + nam + " list items selected.\nPlease select some");
    return  result;
}

function clearboxes()  {
    var els = top.List.document.listform.elements;
    for  (var cnt = 0;  cnt < els.length; cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "checkbox")
	    tag.checked = false;
    }
}

//function gethidden(s)  {
//    var els = document.listform.elements;
//    for  (var cnt = 0;  cnt < els.length; cnt++)  {
//	var tag = els[cnt];
//	if  (tag.type == "hidden"  &&  tag.name == s)
//	    return  tag.value;
//    }
//    return  null;
//}

//========================================================================
//
// View functions
//
//------------------------------------------------------------------------

function viewclicked(prog)  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var jnumlist = new Array(), jpnumlist = new Array(), novplist = new Array(), norvlist = new Array();
    for (var elcnt = 0; elcnt < joblist.length; elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 3) != 3)  {
	    if  (!(perm & 1))
		novplist = novplist.concat(num);
	    if  (!(perm & 2))
		norvlist = norvlist.concat(num);
	}
	else  if  (perm & 0x8000)
		jpnumlist = jpnumlist.concat(num);
	else
		jnumlist = jnumlist.concat(num);
    }
    if  (novplist.length > 0  ||  norvlist.length > 0)  {
	var msg = "You do not have permisssion to view some of the jobs.\n";
	if  (novplist.length > 0)  {
	    msg += "\nFor the following you do not have view permission for\n";
	    msg += "other user's jobs.\n\n";
	    msg += novplist.join(", ");
	}
	if  (norvlist.length > 0)  {
	    msg += "\nFor the following you do not have view permission for\n";
	    msg += "jobs on other hosts.\n\n";
	    msg += norvlist.join(", ");
	}
	alert(msg);
    }

    var url = makenewurl(prog);

    var jpcnt, jp;
    for (jpcnt = 0;  jpcnt < jpnumlist.length;  jpcnt++)  {
	jp = jpnumlist[jpcnt];
	dispvwin(url, jp, "1");
    }
    for (jpcnt = 0;  jpcnt < jnumlist.length;  jpcnt++)  {
	jp = jnumlist[jpcnt];
	dispvwin(url, jp, "");
    }
}

//========================================================================
//
//  Delete functions
//
//------------------------------------------------------------------------

function delclicked()  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var nplist = new Array(), plist = new Array(), nodplist = new Array(), nordlist = new Array();
    for (var elcnt = 0; elcnt < joblist.length; elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 6) != 6)  {
	    if  (!(perm & 4))
		nodplist = nodplist.concat(num);
	    if  (!(perm & 2))
		nordlist = nordlist.concat(num);
	}
	else  if  (perm & 0x4000)
		plist = plist.concat(num);
	else
		nplist = nplist.concat(num);
    }
    if  (nodplist.length > 0  ||  nordlist.length > 0)  {
	var msg = "You do not have permisssion to delete some of the jobs.\n";
	if  (nodplist.length > 0)  {
	    msg += "\nFor the following you do not have delete permission for\n";
	    msg += "other user's jobs.\n\n";
	    msg += nodplist.join(", ");
	}
	if  (nordlist.length > 0)  {
	    msg += "\nFor the following you do not have delete permission for\n";
	    msg += "jobs on other hosts.\n\n";
	    msg += nordlist.join(", ");
	}
	alert(msg);
	return;
    }
    if  (nplist.length > 0)  {
	    var msg = "The following jobs have not been printed.\n\n";
	    msg += nplist.join(", ");
	    msg += "\n\nPlease confirm that they are to be deleted.";
	    if  (confirm(msg))
		    plist = plist.concat(nplist);
    }
    if  (plist.length == 0)
	return;
    vreswin(makenewurl("sqdcgi") + esc_itemlist(plist), 300, 400);
}

//========================================================================
//
//  Change utils, mostly form building
//
//------------------------------------------------------------------------

// Check that jobs are changable, and build an argument suitable
// for change forms

function make_changearg(jl, descr)  {
    var oklist = new Array(), nocplist = new Array(), norclist = new Array();
    for (var elcnt = 0; elcnt < jl.length; elcnt++)  {
	var el = jl[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 10) != 10)  {
	    if  (!(perm & 8))
		nocplist = nocplist.concat(num);
	    if  (!(perm & 2))
		norclist = norclist.concat(num);
	}
	else
		oklist = oklist.concat(num);
    }
    if  (nocplist.length > 0  ||  norclist.length > 0)  {
	var msg = "You do not have permisssion to change some of the " + descr + "s.\n";
	if  (nocplist.length > 0)  {
	    msg += "\nFor the following you do not have change permission for\n";
	    msg += "other user's " + descr + "s.\n\n";
	    msg += nocplist.join(", ");
	}
	if  (norclist.length > 0)  {
	    msg += "\nFor the following you do not have change permission for\n";
	    msg += descr + "s on other hosts.\n\n";
	    msg += norclist.join(", ");
	}
	alert(msg);
	return  "";
    }
    return  oklist.join(',');
}

function clearchanges()  {
    for  (var el in Changedfields)
	Changedfields[el] = false;
    return  true;
}

function makeclick(nam)  {
    return  function()  {  Changedfields[nam] = true;  }
}

function update_corr_text(form, name, val)  {
    var els = form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "text"  &&  tag.name == name)  {
	    tag.value = val;
	    Changedfields[name] = true;	// Do we need this???
	    return;
	}
    }
}

function update_select(name)  {
    var form = document.js_func_form;
    var els = form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "select-one"  &&  tag.name == name)  {
	    var ind = tag.name.lastIndexOf("_upd");
	    update_corr_text(form, tag.name.substr(0, ind), tag.options[tag.selectedIndex].value);
	    return;
	}
    }
}

function makeupd(nam)  {
    return  function()  { update_select(nam); }
}

function record_formchanges(form)  {
    Changedfields = new Object();
    var els = form.elements;
    for (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	switch  (tag.type)  {
	case  "checkbox":
	case  "radio":
//	    tag.onclick = new Closure(function()  { Changedfields[name] = true; }, tag);
	    tag.onclick = makeclick(tag.name);
	    break;
	case  "select-one":
	    if  (tag.name.lastIndexOf("_upd") > 0)  {
		tag.onchange = makeupd(tag.name);
		break;
	    }
	case  "text":
//	    tag.onchange = new Closure(function()  { Changedfields[name] = true; }, tag);
	    tag.onchange = makeclick(tag.name);
	    break;
	}
    }
}

function updboxsel(d, name, list)  {
    d.writeln("<select name=", name, "_upd size=1>");
    for  (var cnt = 0;  cnt < list.length;  cnt++)  {
	var fld = list[cnt];
	d.writeln("<option value=\"", fld, "\">", (fld == ""? "(none)": fld));
    }
    d.writeln("</select>");
}

function fnumsel(doc, nam, begin, end, step, deflt)
{
    doc.writeln("<select name=", nam, " size=1>");
    for  (var cnt = begin;  cnt <= end;  cnt += step)  {
	doc.write("<option");
	if  (cnt == deflt)
	    doc.write(" selected");
	doc.writeln(" value=", cnt, ">", Math.floor(cnt/step));
    }
    doc.writeln("</select>");
}

function fnumselect(doc,nam,begin,end,deflt)
{
    if  (end - begin > 1000)  {
	fnumsel(doc, nam + "_1000", rounddown(begin, 1000), rounddown(end, 1000), 1000, rounddown(deflt, 1000));
	fnumselect(doc, nam, 0, 999, modulo(deflt, 1000));
    }
    else  if  (end - begin > 100)  {
	fnumsel(doc, nam + "_100", rounddown(begin, 100), rounddown(end, 100), 100, rounddown(deflt, 100));
	fnumselect(doc, nam, 0, 99, modulo(deflt, 100));
    }
    else  if  (end - begin > 10)  {
	fnumsel(doc, nam + "_10", rounddown(begin, 10), rounddown(end, 10), 10, rounddown(deflt, 10));
	fnumselect(doc, nam, 0, 9, modulo(deflt, 10));
    }
    else
        fnumsel(doc, nam, begin, end, 1, deflt);
}

function hourtimeout(doc, nam, deflt)
{
    fnumselect(doc, nam + "_d", 0, Math.floor(32767/24), Math.floor(deflt/24));
    doc.write("days");
    fnumsel(doc, nam + "_h", 0, 23, 1, modulo(deflt, 24));
    doc.write("hours");
}

function timedelay(doc, nam, deflt)
{
    fnumselect(doc, nam + "_d", 0, 366, Math.floor(deflt/(24*3600)));
    doc.write("days");
    fnumselect(doc, nam + "_h", 0, 23, Math.floor(modulo(deflt, 24*3600)/3600));
    doc.write(":");
    fnumselect(doc, nam + "_m", 0, 59, Math.floor(modulo(deflt, 3600)/60));
    doc.write(":");
    fnumselect(doc, nam + "_s", 0, 59, modulo(deflt, 60));
}

function prtdate(dat)  {
    var dw = [ "Sun", "Mon", "Tues", "Wednes", "Thurs", "Fri", "Satur" ];
    var mn = [ "January", "February", "March", "April", "May", "June",
	     "July", "August", "September", "October", "November", "December" ];
    // Kludge to get round Netscape Bug
    if  (dat.getTimezoneOffset() == -1440)
	return dw[dat.getUTCDay()] + "day " + dat.getUTCDate() + " " + mn[dat.getUTCMonth()];
    return dw[dat.getDay()] + "day " + dat.getDate() + " " + mn[dat.getMonth()];
}

function dateto(doc, deftim, nam, ndays, plussec)
{
    doc.writeln("<select name=", nam, "_d size=1>");
    var today = new Date();
    var todaymid = roundtomidday(today.getTime());
    var nday = new Date(deftim);
    var ndaymid = roundtomidday(deftim);
    for  (var cnt = 0;  cnt < ndays;  cnt++)  {
	doc.write("<option ");
	if  (todaymid == ndaymid)
	    doc.write("selected ");
	doc.writeln("value=", cnt, ">", prtdate(today));
	deftim += 3600000 * 24;
	nday.setTime(deftim);
	todaymid += 3600000 * 24;
	today.setTime(today.getTime() + 3600000 * 24);
    }
    doc.writeln("</select>");
    fnumselect(doc, nam + "_h", 0, 23, nday.getHours());
    doc.write(":");
    fnumselect(doc, nam + "_m", 0, 59, nday.getMinutes());
    if  (plussec)  {
	doc.write(":");
	fnumselect(doc, nam + "_s", 0, 59, nday.getSeconds());
    }
}

function nprefix(n)  {
    var ind = n.indexOf('_');
    return  ind >= 0? n.substr(0, ind): n;
}

function suffmult(n) {
    var ind = n.indexOf('_');
    if  (ind < 0)
	return  1;
    switch  (n.charAt(ind+1))  {
    default:
	return  1;
    case  'd':
	return  3600 * 24;
    case  'h':
	return  3600;
    case  'm':
	return  60;
    case  's':
	return  1;
    }
}

function listchanged(form)  {
    var cnames = new Array();
    for  (var el in Changedfields)
	if  (Changedfields[el])
	    cnames = cnames.concat(el);

    var resobj = new Object();
    if  (cnames.length == 0)
	return  resobj;

    var prefnobj = new Object();
    for  (var cnt = 0;  cnt < cnames.length;  cnt++)
	prefnobj[nprefix(cnames[cnt])] = true;

    var els = form.elements, nam, val;

    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	switch  (tag.type)  {
	default:
	    continue;
	case  "checkbox":
	    nam = nprefix(tag.name);
	    if  (!prefnobj[nam])
		continue;
	    if  (resobj[nam])  {
		if  (tag.checked)
		    resobj[nam] += parseInt(tag.value);
	    }
	    else
		resobj[nam] = tag.checked? parseInt(tag.value): 0;
	    break;
	case  "radio":
	    if  (!prefnobj[tag.name])
		continue;
	    if  (tag.checked)
		resobj[tag.name] = tag.value;
	    break;
	case  "text":
	    if  (!prefnobj[tag.name])
		continue;
	    resobj[tag.name] = tag.value;
	    break;
	case  "select-one":
	    if  (tag.name.lastIndexOf("_upd") >= 0)
		continue;
	    nam = nprefix(tag.name);
	    if  (!prefnobj[nam])
		continue;
	    val = tag.options[tag.selectedIndex].value * suffmult(tag.name);
	    if  (resobj[nam])
		resobj[nam] += val;
	    else
		resobj[nam] = val;
	    break;
	}
    }
    return  resobj;
}

//  May need to grab selections if a guy has said he wants it but hasn't
//  changed the values from defaults.

function getdefselected(form, nam)  {
    var els = form.elements, val = 0;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "select-one"  &&  nprefix(tag.name) == nam)
	    val += tag.options[tag.selectedIndex].value * suffmult(tag.name);
    }
    return  val;
}

// Wrap html construct such as "H1" round string.

function htmlwrite(ht, str)
{
    return  "<" + ht + ">" + str + "</" + ht + ">";
}

// Initialise change form with name "nam", title "tit", background colour "bgcol"
// header "hdr", width "wid" and height "ht".

function initformwnd(nam, tit, bgcol, hdr, wid, ht, cchk)  {
    var w = open("", nam, "width=" + wid + ",height=" + ht);
    w.document.writeln(htmlwrite("head", htmlwrite("title", tit)));
    if  (bgcol != "")
	w.document.writeln("<body bgcolor=\"", bgcol, "\">");
    else
	w.document.writeln("<body>");
    w.document.writeln("<script src=\"", Self_location, "\"></script>");
    w.document.write(htmlwrite("h1", hdr));
    timer_setup(w, false, cchk);
    return  w;
}

// Also stuff the word "Change" in from of title and header.

function chgformwnd(nam, tit, bgcol, hdr, wid, ht, cchk)  {
    return  initformwnd(nam, "Change " + tit, bgcol, "Change " + hdr, wid, ht, cchk);
}

// Return a suitable string to start a change form and table.

function chngformstart(d, cf)  {
    d.writeln("<form name=js_func_form method=get onSubmit=\"return ", cf, "();\">");
    d.writeln("<table>");
}

// Return a suitable string to end a change form and table.

function chngformend(doc, joblist, pn)  {
    endtable(doc);
    action_buttons(doc, "Make Changes");
    doc.writeln("<input type=reset onclick=\"return clearchanges();\">");
    hidden_value(doc, "js_itemlist", joblist);
    hidden_value(doc, "js_url", makenewurl(pn));
    doc.writeln("</form>\n<script>\nrecord_formchanges(document.js_func_form);\n</script>\n</body>");
}

function endrow(doc)
{
    doc.writeln("</td></tr>");
}

function endtable(doc)
{
    endrow(doc);
    doc.writeln("</table>");
}

function nextcol(doc)
{
    doc.write("</td><td>");
}

function nextcol_cs(doc, n)
{
    doc.write("</td><td colspan=" + n + ">");
}

function tabrowleft(doc, endlast)  {
    if  (endlast)
	endrow(doc);
    doc.writeln("<tr align=left><td>");
}

function tabrowleft_cs(doc, endlast, n)  {
    if  (endlast)
	endrow(doc);
    doc.writeln("<tr align=left><td colspan=" + n + ">");
}

function input_fnumselect(doc, descr, name, mn, mx, df)  {
    doc.writeln(descr);
    nextcol(doc);
    fnumselect(doc, name, mn,  mx, df);
}

function argpush(al, nam, val)  {
    return  al + '&' + escape(nam + '=' + val);
}

function vreswin(cmd, width, height)  {
    var w = open(cmd, "jsreswin", "width=" + width + ",height=" + height);
    timer_setup(w, false, 5000);
    return  false;
}

//========================================================================
//
//	Format changes
//
//------------------------------------------------------------------------

function do_formats()  {
    invoke_opt_prog('listformat');
}

// Lay down radio buttons for left/right/centre

function lf_align_ins(letter, deflt_al, albut, descr)
{
    radio_button(document, "al_" + letter, albut, descr, deflt_al == albut);
}

// Lay down a button to add format code "letter", align "align" and given
// description.

function format_button(letter, descr)  {
    document.write("<input type=button name=", letter, ' value="', fixdescr(descr),
		   '" onclick="format_click(document.fmtform.', letter, ');">\n');
}

function list_format_code(letter, align, descr)  {
    tabrowleft(document, false);
    format_button(letter, descr);
    nextcol(document);
    document.writeln("Align");
    lf_align_ins(letter, align, 'L', "Left");
    lf_align_ins(letter, align, 'C', "Centre");
    lf_align_ins(letter, align, 'R', "Right");
    endrow(document);
    Format_lookup[letter] = descr;
}

function existing_formats(def_fmt)  {
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var  ef = cooklist.format, hdr = "";
    if  (typeof ef != "string")  {
	ef = def_fmt;
	hdr = "(default) ";
    }
    document.writeln("</table><hr><h2>Previous ", hdr, "formats</h2><table>");
    var cnt = 0, lnum = 0;
    while  (lnum < ef.length)  {
	var al = ef.substr(lnum, 1);
	lnum++;
	if  (lnum >= ef.length)
	    break;
	if  (al != 'L'  &&  al != 'R'  &&  al != 'C')
	    continue;
	var fmt = ef.substr(lnum, 1);
	lnum++;
	var descr = Format_lookup[fmt];
	fmt += 'orig';
	cnt++;
	tabrowleft(document, false);
	document.write(cnt);
	nextcol(document);
	format_button(fmt, descr);
	nextcol(document);
	document.writeln("Align");
	lf_align_ins(fmt, al, 'L', "Left");
	lf_align_ins(fmt, al, 'C', "Centre");
	lf_align_ins(fmt, al, 'R', "Right");
	endrow(document);
    }
}

function find_alchecked(letter)
{
    var els = document.fmtform.elements;
    var allet = "al_" + letter;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "radio" && tag.name == allet  &&  tag.checked)
	    return  tag.value;
    }
    return  "L";
}

// Clicking a button, remember which button and the alignment

function format_click(letterbutt)  {
    if  (Fmtbuttarray.length == 0)
	Fmtreswin = open("", "fmtbutts", "width=400,height=500,resizable=yes,scrollbars=yes");
    else
	Fmtreswin.document.open();	//  Should clear existing one
    var lett = letterbutt.name;		//  (May have suffix "orig" if from existing
    var algn = find_alchecked(lett);
    Fmtbuttarray = Fmtbuttarray.concat(algn + lett.substr(0,1) + letterbutt.value);

    //  Now display the result

    var doc = Fmtreswin.document;
    doc.writeln("<head><title>Display format codes</title></head><body><h1>Display codes</h1>");
    doc.writeln("<table><tr align=left><th>Code</th><th>Al</th><th>Descr</th></tr>");
    for  (var cnt = 0;  cnt < Fmtbuttarray.length;  cnt++)  {
	var ent = Fmtbuttarray[cnt];
	lett = ent.substr(1, 1);
	algn = ent.substr(0, 1);
	ent = ent.substr(2);
	tabrowleft(doc, false);
	doc.write(lett);
	nextcol(doc);
	doc.write(algn);
	nextcol(doc);
	doc.write(ent);
	endrow(doc);
    }
    doc.writeln("</table>");
    Fmtreswin.focus();
}

// Store away the formats set up

function setformats(ndays)  {
    if  (Fmtbuttarray.length == 0)  {
	alert("No formats selected yet");
	return  false;
    }
    Fmtreswin.close();
    var fmtstring = "";
    for  (var cnt = 0;  cnt < Fmtbuttarray.length;  cnt++)  {
	var  ent = Fmtbuttarray[cnt];
	fmtstring += ent.substr(0, 2);
    }
    Fmtbuttarray = new Array();
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    cooklist.format = fmtstring;
    storecookies(cooklist, pname, ndays);
    invokeprog(prog, cooklist);
    return  false;		// Not needed as invokeprog doesn't return - but....
}

// Cancel what we've done if anything.

function cancformats()  {
    if  (Fmtbuttarray.length != 0)  {
	Fmtreswin.close();
	Fmtbuttarray = new Array();
    }
    history.go(-1);
    return  false;
}

function resetformat(ndays)  {
    if  (Fmtbuttarray.length != 0)  {
	Fmtreswin.close();
	Fmtbuttarray = new Array();
    }
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var newcooks = new Object();
    var arr = new Array();
    for (var el in cooklist)  {
	if  (el != "format")
	    newcooks[el] = cooklist[el];
	arr.push(el);
    }
    storecookies(newcooks, pname, ndays);
    invokeprog(prog, newcooks);
    return  false;
}

//========================================================================
//
//	Option-setting
//
//------------------------------------------------------------------------

function do_options()  {
    invoke_opt_prog("listopts");
}

function optsave(ndays)  {
    var clist = listchanged(document.optform);
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var newcooks = new Object();
    if  (cooklist.usercode)
	newcooks.usercode = cooklist.usercode;
    if  (cooklist.format)
	newcooks.format = cooklist.format;
    for (var el in clist)
	newcooks[el] = clist[el];
    storecookies(newcooks, pname, ndays);
    invokeprog(prog, newcooks);
    return  false;
}

//========================================================================
//========================================================================
//
//	Change functions are product-specific
//
//------------------------------------------------------------------------
//------------------------------------------------------------------------

function vnameok(n, notrem)  {
    var s = n.value;	
    for (var i = 0;  i < s.length;  i++)  {
	var c = s.charAt(i);
	if  ((c < 'A' || c > 'Z')  &&
	     (c < 'a' || c > 'z')  &&
	     c != '_'  &&  (notrem  ||  c != ':')  &&
	     (i == 0 || c < '0' || c > '9'))
	    return  false;
    }
    return  true;
}

function batchlist(descr, permbits, notrun, oneonly)  {
    var  joblist = listclicked();
    if  (joblist.length == 0)
	return  joblist;
    var oklist = new Array(), noklist = new Array(), runlist = new Array(), notrunlist = new Array();
    for  (var elcnt = 0;  elcnt < joblist.length;  elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var jnum = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & permbits) == permbits)  {
	    if  (perm & 0x2000)  {
		if  (notrun)
		    runlist = runlist.concat(jnum);
		else
		    oklist = oklist.concat(jnum);
	    }
	    else  {
		if  (notrun)
		    oklist = oklist.concat(jnum);
		else
		    notrunlist = notrunlist.concat(jnum);
	    }
	}
	else
	    noklist = noklist.concat(jnum);
    }
    if  (runlist.length > 0  ||  notrunlist.length > 0  ||  noklist.length > 0)  {
	if  (runlist.length > 0)
	    alert("The following jobs are still running so you cannot " + descr + ".\n" + runlist.join(", "));
	if  (notrunlist.length > 0)
	    alert("The following jobs are not running so you cannot " + descr + ".\n" + notrunlist.join(", "));
	if  (noklist.length > 0)
	    alert("The following jobs do not have the right permission, you cannot " + descr + ".\n" + noklist.join(", "));
	return  new Array();
    }
    if  (oneonly  &&  oklist.length > 1)  {
	alert("You can only select one job at a time for this operation");
	return  new Array();
    }
    return  oklist;
}
    
function jobops(code, descr, permbits, notrun)  {
    var  joblist = batchlist(descr, permbits, notrun, false);
    if  (joblist.length == 0)
	return  false;
    return  vreswin(makenewurl("btjdcgi") + "&" + code + esc_itemlist(joblist), 460, 230);
}

function  permbox(ucode, code, checked)  {
    nextcol(document);
    document.write("<input type=checkbox name=", ucode, code, " value=1");
    if  (checked)
	    document.write(" checked");
    document.writeln(">");
}

function btperm_checkboxes(descr, code, user, grp, oth)  {
    tabrowleft(document, false);
    document.write(descr);
    permbox('U', code, user);
    permbox('G', code, grp);
    permbox('O', code, oth);
    endrow(document);
}

function gb_cancperm() {
    history.go(-1);
    return  false;
}

//========================================================================
//========================================================================
//
//	Batch - job hacking
//
//------------------------------------------------------------------------
//------------------------------------------------------------------------

function gbj_delete()  {
    jobops("delete", "Delete them", 4, true);
}

function gbj_runnable()  {
    jobops("ready", "Set them runnable", 8, true);
}

function gbj_cancel()  {
    jobops("canc", "Set them cancelled", 8, true);
}

function gbj_advance()  {
    jobops("adv", "Advance their times", 8, true);
}

function gbj_force(andadv)  {
    jobops(andadv? "goadv": "go", andadv? "Force run and advance them": "Force them to run", 0x40, true);
}

function killtype_button(doc, val, descr, isdef)
{
    radio_button(doc, "killtype", val, descr, isdef);
}

function gbj_kill()  {
    var joblist = batchlist("Kill them", 0x40, false, false);
    if  (joblist.length == 0)
	return;
    var w = initformwnd("killw", "Kill Xi-Batch jobs", "#DFDFFF", "Kill Jobs", 400, 300, 5000);
    var d = w.document;
    d.writeln("<form name=killform method=get onSubmit=\"return gbj_kill2('",
	    makenewurl("btjdcgi"), "');\">\n<table>");
    tabrowleft(d, false);
    killtype_button(d, 1, "Hangup signal", false);
    tabrowleft(d, true);
    killtype_button(d, 2, "Interrupt", true);
    tabrowleft(d, true);
    killtype_button(d, 3, "Quit", false);
    tabrowleft(d, true);
    killtype_button(d, 9, "KILL", false);
    tabrowleft(d, true);
    killtype_button(d, 15, "Terminate", false);
    endtable(d);
    action_buttons(d, "Kill");
    hidden_value(d, "Joblist", joblist.join(','));
    d.writeln("</form>\n</body>");
}

function gbj_kill2(url)  {
    var els = document.killform.elements;
    var joblist = new Array(), killtype = 15;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	switch  (tag.type)  {
	case  "hidden":
	    joblist = tag.value.split(',');
	    break;
	case  "radio":
	    if  (tag.checked)
		killtype = tag.value;
	    break;
	}
    }
    if  (joblist.length == 0)
	return  false;
    document.location = url + "&kill" + escape('=') + killtype + esc_itemlist(joblist);
    return  false;
}

function gbj_mailwrt()  {
    var joblist = batchlist("Set mail/write flags on them", 8, true, false);
    if  (joblist.length == 0)
	return;
    var w = chgformwnd("mailwrt", "mail/write flags for jobs", "#DFDFFF",
		       "mail and write markers for job(s)", 500, 200, 5000);
    var d = w.document;
    chngformstart(d, "gbj_mailwrt2");
    tabrowleft(d, false);
    checkbox_item(d, "mail", "Mail message on completion");
    tabrowleft(d, true);
    checkbox_item(d, "write", "Write message to terminal on completion");
    endtable(d);
    action_buttons(d, "Set flags");
    hidden_value(d, "js_itemlist", joblist.join(','));
    hidden_value(d, "js_url", makenewurl("btjccgi"));
    d.writeln("</form>\n</body>");
}

function gbj_mailwrt2()  {
    var  marg = escape("mail=" + (document.js_func_form.mail.checked? 't': 'f'));
    var  warg = escape("write=" + (document.js_func_form.mail.checked? 't': 'f'));
    document.location = get_chgurl() + '&' + marg + '&' + warg + get_chgitemlist();
    return  false;
}

function gbj_perms()  {
    var joblist = batchlist("change permissions on them", 0x20, true, true);
    if  (joblist.length == 0)
	return;
    document.location = urlandfirstarg(document.location) + '&listperms' + escape(':' + joblist[0]);
    return;
}

function gbj_perms2()  {
    var clist = listchanged(document.js_func_form);
    var sets = "", unsets = "";
    for  (var el in clist)
	if  (clist[el])
	    sets += el;
        else
	    unsets += el;
    if  (sets.length == 0  &&  unsets.length == 0)
	return  gb_cancperm();
    if  (sets.length == 0)
	sets = "-";
    if  (unsets.length == 0)
	unsets = "-";
    var newloc = makenewurl("btjdcgi") + "&perms&" + escape(document.js_func_form.jnum.value) + "&" + sets + "&" + unsets;
    timer_setup(open(newloc, "pres", "width=460,height=230"), true, 3000);
    return  false;
}

//======================================================================================
//
// For more complicated parameters, where the existing settings need to be displayed
// we do things in 3 steps.
//
//	1.	Initial step invoked from list screen
//	2.	btjcgi invokes step2 invokation directly in
//		output containing existing parameters
//	3.	Final step invokes btjccgi.
//
//======================================================================================
//
//	Kick off with some shared routines
//
//--------------------------------------------------------------------------------------

function gbj_step1(permdescr, permbit, opcode)  {
    var joblist = batchlist(permdescr, permbit, true, true);
    if  (joblist.length == 0)
	return  false;
    document.location = urlandfirstarg(document.location) + '&' + opcode + escape(':' + joblist[0]);
    return  false;
}

//======================================================================================
//
// Title/pri/ll helpers
//
//======================================================================================


function gbj_titprill()  {
    gbj_step1("change title etc on them", 8, 'listtitprill');
}

// Called from output of btjcgi (etc) with job number, existing title, pris
// (as array current/def/min/max, load level, current ci, can change ll,
// queue name list, command interpreter name list

function gbj_titprill2(jnum, title, pris, ll, ciname, cansetll, qlist, cilist)  {
    var	qname = "", tit = title, ind;

    //  Possibly split title into queue name and title

    if  ((ind = title.indexOf(':')) >= 0)  {
	qname = title.substr(0, ind);
	tit = title.substr(ind+1);
    }

    //  Queue line with selection box to insert standard one

    tabrowleft(document, false);
    document.write("Queue:");
    nextcol(document);
    text_box_init(document, "queue", 20, 40, qname);
    nextcol(document);
    document.write("Possible:");
    nextcol(document);
    updboxsel(document, "queue", qlist);

    //  Title line

    tabrowleft(document, true);
    document.write("Title:");
    nextcol_cs(document, 3);
    text_box_init(document, "title", 60, 255, tit);

    //  Priority line

    tabrowleft(document, true);
    document.writeln("Priority:");
    nextcol_cs(document, 3);
    fnumselect(document, "pri", pris[2], pris[3], pris[0]);

    //  Load level line

    tabrowleft(document, true);
    document.writeln("Load level:");
    nextcol_cs(document, 3);
    if  (cansetll)
	fnumselect(document, "ll", 1, 30000, ll);
    else
	document.writeln("Set by command interpreter");
    
    //  Command interpreter

    tabrowleft(document, true);
    document.write("Cmd int:");
    nextcol(document);
    text_box_init(document, "interp", 15, 15, ciname);
    document.write("Possible:");
    nextcol(document);
    updboxsel(document, "interp", cilist);
    endrow(document);
}

function gbj_titprill3()  {
    var clist = listchanged(document.js_func_form);
    var arglist = "", errs = 0;
    for  (var el in clist)
	switch  (el)  {
	case  "queue":
	case  "title":
	case  "interp":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "pri":
	    if  (clist[el] <= 0)  {
	        alert("Priority cannot be 0 or less");
		errs++;
	        break;
	    }
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "ll":
	    if  (clist[el] <= 0)  {
	        alert("Priority cannot be 0 or less");
		errs++;
	        break;
	    }
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	}
    if  (arglist.length == 0  ||  errs > 0)  {
	alert("No changes made");
	return  false;
    }
    var newloc = makenewurl("btjccgi") + arglist + get_chgitemlist();
    timer_setup(open(newloc, "titpres", "width=460,height=230"), true, 3000);
    return  false;
}

//======================================================================================
//
// Process parameters
//
//======================================================================================

function gbj_procpar()  {
    gbj_step1("change process parameters on them", 8, 'listprocpar');
}

function umaskbits(doc, umask, bit, name)  {
    var  nams = [ "R", "W", "X" ];
    nextcol(doc);
    for  (var cnt = 0;  cnt < 3;  cnt++)  {
	doc.write("<input type=checkbox name=umask_", name, nams[cnt], " value=", bit);
	if  (umask & bit)
	    doc.write(" checked");
	doc.writeln(">", nams[cnt]);
	bit >>= 1;
    }
}

function gbj_procpar2(jnum, direc, umask, ulimit, eranges, noadv, expflg, remrun)  {

    //  Kick off with directory

    tabrowleft(document, false);
    document.write("Directory:");
    nextcol_cs(document, 3);
    text_box_init(document, "directory", 40, 255, direc);

    //  Umask is a table of bits

    tabrowleft(document, true);
    document.write("Umask turn OFF user:");
    umaskbits(document, umask, 0400, "U");
    tabrowleft(document, true);
    document.write("Group:");
    umaskbits(document, umask, 0040, "G");
    tabrowleft(document, true);
    document.write("Others:");
    umaskbits(document, umask, 0004, "O");

    //  Ulimit is just a text box

    tabrowleft(document, true);
    document.write("Ulimit:");
    nextcol(document);
    text_box_init(document, "ulimit", 10, 20, "0x" + ulimit.toString(16));

    //  Exit codes

    tabrowleft(document, true);
    document.write("Normal Exit:");
    nextcol(document);
    fnumselect(document, "nel", 0, 255, eranges[0]);
    nextcol(document);
    fnumselect(document, "neu", 0, 255, eranges[1]);
    tabrowleft(document, true);
    document.write("Error Exit:");
    nextcol(document);
    fnumselect(document, "eel", 0, 255, eranges[2]);
    nextcol(document);
    fnumselect(document, "eeu", 0, 255, eranges[3]);

    // No advance on error

    tabrowleft_cs(document, true, 3);
    checkbox_item_chk(document, "noadv", "Suppress time advance on error", noadv);

    // Export

    tabrowleft_cs(document, true, 3);
    radio_button(document, "export", "l", "Job is purely local to host", !(expflg || remrun));
    tabrowleft_cs(document, true, 3);
    radio_button(document, "export", "e", "Job is visible but not runnable remotely", expflg && !remrun);
    tabrowleft_cs(document, true, 3);
    radio_button(document, "export", "r", "Job is runnable remotely", remrun);
    endrow(document);
}

function gbj_procpar3()  {
    var clist = listchanged(document.js_func_form);
    var arglist = "", errs = 0;
    for  (var el in clist)
	switch  (el)  {
	case  "directory":
	case  "umask":
	case  "nel":case "neu":
	case  "eel":case "eeu":
	case  "export":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "noadv":
	    arglist = argpush(arglist, el, clist[el]? "y": "n");
	    break;
	case  "ulimit":
	    arglist = argpush(arglist, el, parseInt(clist[el]));
	    break;
	}
    if  (arglist.length == 0  ||  errs > 0)  {
	alert("No changes made");
	return  false;
    }
    var newloc = makenewurl("btjccgi") + arglist + get_chgitemlist();
    timer_setup(open(newloc, "ppres", "width=460,height=230"), true, 3000);
    return  false;
}

//========================================================================
//
//	Time parameters
//
//========================================================================

function gbj_times()  {
    gbj_step1("change time parameters on them", 8, 'listtime');
}

function tselecttype(d, v, nam, descr, nlist)  {
    d.write(descr);
    nextcol(d);
    d.writeln("<select name=", nam, " size=1>");
    for  (var cnt = 0;  cnt < nlist.length;  cnt++)  {
	d.write("<option");
	if  (cnt == v)
	    d.write(" selected");
	d.writeln(" value=", cnt, ">", nlist[cnt]);
    }
    d.writeln("</select>");
}

function putdavoid(d, s, nlist, v)  {
    //    d.write("<tt>");
    for  (var shft = s, cnt = 0;  cnt < 4;  shft++, cnt++)  {
	var bitf = 1 << shft;
	d.write("<input type=checkbox name=avdays value=", bitf);
	if  (v & bitf)
	    d.write(" checked");
	d.writeln(">", nlist[cnt]);
    }
    //  d.writeln("</tt>");
}

function gbj_times2(jnum, tset, nextt, reptype, reprate, mday, nvaldays, nposstype)  {

    //  Kick off with "time constraint currently set"

    tabrowleft(document, false);
    document.write("<td>");
    checkbox_item_chk(document, "tset", "Job has time set", tset);

    //  Set up selection for next time

    tabrowleft_cs(document, true, 2);
    var today = new Date();
    dateto(document, tset? nextt * 1000: today.getTime() + 3600000, "time", 60, false);
    tabrowleft(document, true);
    tselecttype(document, reptype, "repeat", "Repeat style",
		["Once & delete", "Once & retain", "Minutes",
		"Hours", "Days", "Weeks", "Months relative beginning",
		"Months relative end", "Years" ]);
    tabrowleft(document, true);
    document.write("Repeat number of units");
    nextcol(document);
    fnumsel(document, "rate", 1, 20, 1, reprate > 20? 20: reprate);
    tabrowleft_cs(document, true, 2);
    document.write("Month day from beginning or end 1=first or last");
    nextcol(document);
    fnumsel(document, "mday", 1, 18, 1, mday > 18? 18: mday);
    tabrowleft(document, true);
    document.write("Avoiding days");
    nextcol(document);
    putdavoid(document, 0, ["Sun", "Mon", "Tue", "Wed"], nvaldays);
    tabrowleft(document, true);
    nextcol(document);
    putdavoid(document, 4, ["Thu", "Fri", "Sat", "Hol"], nvaldays);
    tabrowleft(document, true);
    nextcol(document);
    tselecttype(document, nposstype, "ifnp", "If not possible",
		["Skip", "Delay current", "Delay all", "Catchup"]);
    endrow(document);
}

function gbj_times3()  {
    var clist = listchanged(document.js_func_form);
    var arglist = "", errs = 0, nn, today;
 mainloop:
    for  (var el in clist)
	switch  (el)  {
	case  "tset":
	    if  (!clist[el])  {
		arglist = argpush("", "time", 0);
		break mainloop;
	    }
	    if  (clist["time"])
		break;
	    nn = getdefselected(document.js_func_form, "time");
	    today = new Date();
	    nn += Math.floor((today.getTime() / (24 * 3600000))) * 24 * 3600;
	    arglist = argpush(arglist, "time", nn);
	    continue;
	case  "time":
	    nn = clist["time"];
	    today = new Date();
	    nn += Math.floor((today.getTime() / (24 * 3600000))) * 24 * 3600;
	    arglist = argpush(arglist, "time", nn);
	    continue;
	case  "rate":
	case  "mday":
	case  "repeat":
	case  "avdays":
	case  "ifnp":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	}
    if  (arglist.length == 0  ||  errs > 0)  {
	alert("No changes made");
	return  false;
    }
    var newloc = makenewurl("btjccgi") + arglist + get_chgitemlist();
    timer_setup(open(newloc, "tres", "width=460,height=230"), true, 3000);
    return  false;
}

//========================================================================
//
//	Work stuff for arguments, redirections, conditions, assignments
//
//========================================================================

var Op_data = new Object();

function gb_cancchlist()  {
    if  (typeof Op_data.editw == "object"  &&  !Op_data.editw.closed)  {
	    Op_data.editw.close();
	    Op_data.editw = void 0;
    }
    if  (typeof Op_data.resultlist == "object" && Op_data.resultlist.length != 0)  {
	Op_data.resultwin.close();
	Op_data.resultlist = new Array();
    }
    history.go(-1);
    return  false;
}

function clearlistch()  {
    if  (Op_data.resultlist.length != 0)  {
	Op_data.resultwin.close();
	Op_data.resultlist = new Array();
    }
    return  true;
}

function appendtoresult(win, item)  {
    var opd = win.Op_data;
    if  (opd.resultlist.length == 0)
	opd.resultwin = open("",
			     "opres",
			     "width=" + opd.resw_wid +
			     ",height=" + opd.resw_ht +
			     ",resizable=yes,scrollbars=yes");
    else
	opd.resultwin.document.open();	//  Should clear existing one

    // Desperate attempts to work around MS bugs.
    var arr = new Array();
    for  (var cnt = 0;  cnt < opd.resultlist.length;  cnt++)
	arr = arr.concat(opd.resultlist[cnt]);
    arr = arr.concat(item);
    win.Op_data.resultlist = arr;
}

// This is used for redirections/conditions/assignments where
// we use the correct functions automatically

function addtopending(w, item)  {
    appendtoresult(w, item);
    tabledisplay(w);
    return  false;
}

function tabledisplay(win)  {
    var opd = win.Op_data;
    var doc = opd.resultwin.document;
    doc.writeln(htmlwrite("head", htmlwrite("title", opd.title)));
    doc.writeln("<body>");
    doc.writeln(htmlwrite("h1", opd.heading));
    doc.writeln("<table><tr align=left>");
    for  (var hcnt = 0;  hcnt < opd.colhdr.length;  hcnt++)
	doc.writeln("<th>", opd.colhdr[hcnt], "</th>");
    doc.writeln("</tr>");
    for  (var cnt = 0;  cnt < opd.resultlist.length;  cnt++)  {
	tabrowleft(doc, false);
	doc.write(cnt+1);
	nextcol(doc);
	opd.resultlist[cnt].tabdisplay(doc);
	endrow(doc);
    }
    doc.writeln("</table>");
    opd.resultwin.focus();
}

function gbj_step3(clearing)  {

    //  Kill any pending edit window

    if  (typeof Op_data.editw == "object"  &&  !Op_data.editw.closed)  {
	Op_data.editw.close();
	Op_data.editw = void 0;
    }

    //  Close results window, and go back if not clearing
    //  old stuff.

    if  (typeof Op_data.resultlist == 'object' && Op_data.resultlist.length != 0)
	Op_data.resultwin.close();
    else  if  (!clearing)  {
	history.go(-1);
	return  false;
    }

    //  Build new arg list array

    var alist = new Array();
    for  (var cnt = 0;  cnt < Op_data.resultlist.length;  cnt++)  {
	alist = alist.concat(Op_data.resultlist[cnt].argconvert());
    }
    alist = escape('=' + alist.join("\n"));

    //  And now invoke change...

    var newloc = makenewurl("btjccgi") + '&' + Op_data.opcode + alist + get_chgitemlist();
    timer_setup(open(newloc, "ares", "width=460,height=230"), true, 3000);
    return  false;
}


// Work functions for redirections, conds, assignments

function create_create_window(title, heading, colour, wid, ht, endrout)  {
    var w = open("", "createw", "width=" + wid + ",height=" + ht);
    Op_data.editw = w;
    var d = w.document;
    d.writeln(htmlwrite("head", htmlwrite("title", title)));
    d.writeln("<body bgcolor=\"", colour, "\">");
//  d.writeln("<script src=\"", Self_location, "\"></script>");
    d.write(htmlwrite("h1", heading));
    d.writeln("<FORM name=editform method=get onSubmit=\"return window.opener.", endrout, "(window, this);\">");
    return  w;
}

// Make selection box for variables

function variable_select(d)  {
    tabrowleft(d, false);
    d.write("Variable:");
    nextcol(d);
    d.writeln("<select name=varname size=1>");
    var opp = Op_data.possibles;
    for  (var cnt = 0;  cnt < opp.length;  cnt++)
	d.writeln("<option value=\"", opp[cnt], "\">", opp[cnt]);
    d.write("</select>");
    endrow(d);
}

// Make selection box for action

function action_select(d, namearray)  {
    tabrowleft(d, false);
    d.write("Action:");
    nextcol(d);
    d.writeln("<select name=action size=1>");
    for  (var cnt = 1;  cnt < namearray.length;  cnt++)
	d.writeln("<option  value=", cnt, ">", namearray[cnt]);
    d.write("</select>");
    endrow(d);
}

function condassconst(d, name)  {
    tabrowleft(d, false);
    d.write("Value:");
    nextcol(d);
    text_box(d, name, 20, 49);
    endrow(d);
}

function crit_box(d)  {
    tabrowleft_cs(d, false, 2);
    checkbox_item(d, "crit", "Ignore if remote host not running");
    endrow(d);
}

//========================================================================
//
//	Arguments
//
//========================================================================


function gbj_args()  {
    gbj_step1("change arguments on them", 8, 'listargs');
}

function Argument(att)  {
    this.argval = att;
}

function argtostr()  {
    return  this.argval;
}

function argdisplay(doc)  {
    doc.write(fixdescr(this.argval));
}

new Argument("");
Argument.prototype.argconvert = argtostr;
Argument.prototype.tabdisplay = argdisplay;

function find_arg(argn) {
    var nam = "arg" + argn;
    var els = document.js_func_form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "text" && tag.name == nam)
	    return  tag.value;
    }
    return  "";
}

function addarg(argn)  {
    appendtoresult(window, new Argument(find_arg(argn)));
    tabledisplay(window);
    return  false;
}

function gbj_args2(jnum, arglist)  {
    Op_data.title = "Job Arguments";
    Op_data.heading = "New job arguments";
    Op_data.colhdr = ["#", "Argument" ];
    Op_data.existing = arglist;
    Op_data.resw_wid = 400;
    Op_data.resw_ht = 500;
    Op_data.opcode = "args";
    for  (var cnt = 0;  cnt < arglist.length;  cnt++)  {
	tabrowleft(document, false);
        document.write(cnt + 1, ":");
	nextcol(document);
	text_box_init(document, "arg" + cnt, 30, 255, arglist[cnt]);
	nextcol(document);
	submit_button(document, "add" + cnt, "Add to new", "return addarg(" + cnt + ")");
	endrow(document);
    }
    tabrowleft(document, false);
    document.write("New:");
    nextcol(document);
    text_box(document, "argnew", 30, 255);
    nextcol(document);
    submit_button(document, "addnew", "Add to new", "return addarg('new')");
    endrow(document);
    Op_data.resultlist = new Array();
}

//========================================================================
//
//	Redirections
//
//========================================================================

function gbj_io()  {
    gbj_step1("change I/O parameters them", 8, 'listio');
}

function Redirection(attlist)  {
    this.iofd = parseInt(attlist[0]);
    this.action = parseInt(attlist[1]);
    if  (this.action >= 8)
	this.dupfd = parseInt(attlist[2]);
    else
	this.filename = attlist[2];
}

function redirtostr()  {
    var  result = this.iofd + "," + this.action + ",";
    if  (this.action >= 8)
	result += this.action == 8? "0": this.dupfd;
    else
	result += this.filename;
    return  result;
}

function redirdisplay(doc) {
    doc.write(this.iofd, "</td><td>", this.opnames[this.action], "</td><td>");
    switch  (this.action)  {
    case  9:
	    doc.write(this.dupfd);
    case  8:
	    break;
    default:
	    doc.write(this.filename);
	    break;
    }
}

var dummr = new Redirection(["0","0","0"]);
Redirection.prototype.argconvert = redirtostr;
Redirection.prototype.tabdisplay = redirdisplay;
Redirection.prototype.opnames = new Array("??", "Read", "Write", "Append", "R/W", "R/W Append",
					  "Pipe To", "Pipe From", "Close", "Dup" );

function endioedit(w, f)  {
    var Naction = f.ioact.options[f.ioact.selectedIndex].value;
    var Narg3 = 0;
    if  (Naction >= 8)  {
	 if  (Naction > 8)  {
	     if  (!w.opener.isnumeric(f.file.value))  {
		 alert("Dup fd is not numeric");
		 return  false;
	     }
	     Narg3 = parseInt(f.file.value);
	 }
    }
    else  {
	if  (w.opener.isblank(f.file))  {
	    alert("Blank file name");
	    return  false;
	}
	Narg3 = f.file.value;
    }
    w.opener.addtopending(w.opener,
		 new w.opener.Redirection(new Array(f.fd.options[f.fd.selectedIndex].value, Naction, Narg3)));
    w.close();
    return  false;
}

function createio()  {
    var w = create_create_window("Create new redirection", "New Redirection", "#C0C0FF", 400, 300, "endioedit");
    var d = w.document;
    d.writeln("<table>");
    tabrowleft(d, false);
    d.write("File Descriptor:");
    nextcol(d);
    fnumsel(d, "fd", 0, 63, 1, 1);
    tabrowleft(d, true);
    d.writeln("Action:");
    nextcol(d);
    d.write("<select name=ioact size=1>");
    var opn = Redirection.prototype.opnames;
    for  (var cnt = 1;  cnt < opn.length;  cnt++)  {
	d.write("<option");
	if  (cnt == 2)
	    d.write(" selected");
	d.writeln(" value=", cnt, ">", opn[cnt]);
    }
    d.writeln("</select>");
    tabrowleft(d, true);
    d.write("File name:");
    nextcol(d);
    text_box(d, "file", 20, 255);
    endtable(d);
    action_buttons(d, "Create IO");
    d.writeln("</form>");
    return  false;
}

function addio(n)  {
    return  addtopending(window, new Redirection(Op_data.existing[n]));
}

function gbj_io2(jnum, iolist)  {
    Op_data.title = "Job IO";
    Op_data.heading = "New job IO";
    Op_data.colhdr = ["#", "File Dscr", "Action", "File"];
    Op_data.existing = iolist;
    Op_data.resw_wid = 400;
    Op_data.resw_ht = 500;
    Op_data.opcode = "io";

    for  (var cnt = 0;  cnt < iolist.length;  cnt++)  {
	tabrowleft(document, false);
	document.write(cnt + 1, ":");
	nextcol(document);
	var n = new Redirection(iolist[cnt]);
	n.tabdisplay(document);
	nextcol(document);
	submit_button(document, "add" + cnt, "Add to new", "return addio(" + cnt + ")");
	endrow(document);
    }
    tabrowleft(document, false);
    document.write("New:");
    nextcol(document);
    nextcol(document);
    submit_button(document, "addnew", "Add to new", "return createio()");
    endrow(document);
    Op_data.resultlist = new Array();
}

//========================================================================
//
//	Conditions
//
//========================================================================

function gbj_conds()  {
    gbj_step1("change conditions on them", 8, 'listconds');
}

function condtostr()  {
    return  (this.crit? '1': '0') + ',' + this.variable + ',' + this.action + ',' + this.value;
}

function conddisplay(doc)  {
    if  (this.crit)
	doc.write("Y");
    doc.write("</td><td>", this.variable, "</td><td>", this.opnames[this.action], "</td><td>", this.value);
}

function Condition(attlist)  {
    this.crit = parseInt(attlist[0]);
    this.variable = attlist[1];
    this.action = parseInt(attlist[2]);
    this.value = attlist[3];
}

var dummc = new Condition(["0","0","0","0"]);
Condition.prototype.argconvert = condtostr;
Condition.prototype.tabdisplay = conddisplay;
Condition.prototype.opnames = [ "??", "equals", "not equal", "less",
			      "less or equal", "greater", "greater or equal" ];

function endcondedit(w, f)  {
    w.opener.addtopending(w.opener, new w.opener.Condition([f.crit.checked? "1": "0",
						  f.varname.options[f.varname.selectedIndex].value,
						  f.action.options[f.action.selectedIndex].value.toString(10),
						  f.convalue.value]));
    w.close();
    return  false;
}

function createcond()  {
    if  (Op_data.resultlist.length >= 10)  {
	alert("Maximum of 10 conditions reached");
	return  false;
    }
    var w = create_create_window("Create new condition", "New Condition", "#C0C0FF", 400, 300, "endcondedit");
    var d = w.document;
    d.writeln("<table>");
    variable_select(d);
    action_select(d, Condition.prototype.opnames);
    condassconst(d, "convalue");
    crit_box(d);
    d.writeln("</table>");
    action_buttons(d, "Create Cond");
    d.writeln("</form>");
    return  false;
}

function addcond(n)  {
    return  addtopending(window, new Condition(Op_data.existing[n]));
}

function gbj_conds2(jnum, econds, possvars)  {
    Op_data.title = "Job Conditions";
    Op_data.heading = "New job conditions";
    Op_data.colhdr = ["#", "Crit", "Variable", "Compare", "Value" ];
    Op_data.existing = econds;
    Op_data.possibles = possvars;
    Op_data.resw_wid = 400;
    Op_data.resw_ht = 500;
    Op_data.opcode = "conds";
    for  (var cnt = 0;  cnt < econds.length;  cnt++)  {
	tabrowleft(document, false);
	var n = new Condition(econds[cnt]);
	n.tabdisplay(document);
	nextcol(document);
	submit_button(document, "add" + cnt, "Add to new", "return addcond(" + cnt + ")");
	endrow(document);
    }
    tabrowleft_cs(document, false, 4);
    document.write("New Condition:");
    nextcol(document);
    submit_button(document, "addnew", "Add to new", "return createcond()");
    endrow(document);
    Op_data.resultlist = new Array();
}

//========================================================================
//
//	Assignments
//
//========================================================================

function gbj_asses()  {
    gbj_step1("change assignments on them", 8, 'listasses');
}

function Assignment(attlist) {
    this.crit = parseInt(attlist[0]);
    this.flags = parseInt(attlist[1]);
    this.variable = attlist[2];
    this.action = parseInt(attlist[3]);
    this.value = attlist[4];
}

function asstostr()  {
    return (this.crit? '1': '0') + ',' + this.flags + ',' +
	    this.variable + ',' + this.action + ',' + this.value;
}

function assdisplay(doc)  {
    if  (this.crit)
	doc.write("Y");
    doc.write("</td><td>");
    if  (this.flags & 1)
	doc.write('S');
    if  (this.flags & 2)
	doc.write('N');
    if  (this.flags & 4)
	doc.write('E');
    if  (this.flags & 8)
	doc.write('A');
    if  (this.flags & 0x10)
	doc.write('C');
    if  (this.flags & 0x1000)
	doc.write('R');
    doc.write("</td><td>", this.variable, "</td><td>", this.opnames[this.action], "</td><td>");
    if  (this.action < 7)
	doc.write(this.value);
}

var dumma = new Assignment(["0","0","0","0","0"]);
Assignment.prototype.argconvert = asstostr;
Assignment.prototype.tabdisplay = assdisplay;
Assignment.prototype.opnames = [ "??", "assign", "increment", "decrement", "multiply",
			       "divide", "modulus", "assign exit", "assign signal"];

function endassedit(w, f)  {
    var resflags = 0;
    for  (var cnt = 0;  cnt < f.elements.length;  cnt++)  {
	var tag = f.elements[cnt];
	if  (tag.type == "checkbox" && tag.name == "flag" && tag.checked)
	    resflags |= tag.value;
    }
    if  ((resflags & 0x1f) == 0)  {
	alert("You must set one or more flags");
	return  false;
    }
    w.opener.addtopending(w.opener,
		 new w.opener.Assignment([f.crit.checked? "1":"0",
					 resflags.toString(10),
					 f.varname.options[f.varname.selectedIndex].value,
					 f.action.options[f.action.selectedIndex].value.toString(10),
					 f.assvalue.value]));
    w.close();
    return  false;
}

function flag_check(doc, name, val, chk)  {
    doc.write("<input type=checkbox name=flag value=", val);
    if  (chk)
	doc.write(" checked");
    doc.writeln(">", name);
}

function createass()  {
    if  (Op_data.resultlist.length >= 8)  {
	alert("Maximum of 8 assignments reached");
	return  false;
    }
    var w = create_create_window("Create new assignment", "New Assignment", "#C0C0FF", 400, 400, "endassedit");
    var d = w.document;
    d.writeln("<table>");
    variable_select(d);
    tabrowleft(d, false);
    d.write("When to apply:");
    nextcol(d);
    flag_check(d, 'Start', 1, true);
    flag_check(d, 'Normal', 2, true);
    flag_check(d, 'Error', 4, true);
    tabrowleft(d, true);
    nextcol(d);
    flag_check(d, 'Abort', 8, true);
    flag_check(d, 'Cancel', 0x10);
    flag_check(d, 'Reverse', 0x1000, true);
    endrow(d);
    action_select(d, Assignment.prototype.opnames);
    condassconst(d, "assvalue");
    crit_box(d);
    d.writeln("</table>");
    action_buttons(d, "Create Ass");
    d.writeln("</form>");
    return  false;
}

function addass(n)  {
    return  addtopending(window, new Assignment(Op_data.existing[n]));
}

function gbj_asses2(jnum, easses, possvars)  {
    Op_data.title = "Job Assignments";
    Op_data.heading = "New job assignments";
    Op_data.colhdr = ["#", "Crit", "When", "Variable", "Type", "Value" ];
    Op_data.existing = easses;
    Op_data.possibles = possvars;
    Op_data.resw_wid = 400;
    Op_data.resw_ht = 500;
    Op_data.opcode = "asses";
    for  (var cnt = 0;  cnt < easses.length;  cnt++)  {
	tabrowleft(document, false);
	var n = new Assignment(easses[cnt]);
	n.tabdisplay(document);
	nextcol(document);
	submit_button(document, "add" + cnt, "Add to new", "return addass(" + cnt + ")");
	endrow(document);
    }
    tabrowleft_cs(document, false, 5);
    document.write("New Assignment");
    nextcol(document);
    submit_button(document, "addnew", "Add to new", "return createass()");
    endrow(document);
    Op_data.resultlist = new Array();
}

//========================================================================
//========================================================================
//
//	Create job....
//
//========================================================================

function gbj_create(fromfile)  {
    if  (parseInt(top.List.document.listform.privs.value) & 1)  {
	var narg = '&create';
	if  (fromfile)
	    narg = narg + 'f';
	document.location = urlandfirstarg(document.location) + narg;
    }
    else
	alert("Sorry, no permission to create jobs");
    return  false;
}

function gbj_create2(qlist, homed, cilist, fromfile)  {
    document.write("<form name=js_func_form ");
    if  (fromfile)
	document.write("enctype=\"multipart/form-data\" ");
    document.writeln("method=post action=\"", makenewurl("btjcrcgi"), "\">");
    document.writeln("<table>");

    tabrowleft(document, false);
    document.write("Queue:");
    nextcol(document);
    text_box(document, "queue", 20, 40);
    nextcol(document);
    document.write("Possible:");
    nextcol(document);
    updboxsel(document, "queue", qlist);

    tabrowleft(document, true);
    document.write("Title:");
    nextcol_cs(document, 3);
    text_box(document, "title", 60, 255);

    tabrowleft(document, true);
    document.write("Directory:");
    nextcol_cs(document, 3);
    text_box_init(document, "directory", 40, 255, homed);

    tabrowleft(document, true);
    document.write("Cmd int:");
    nextcol(document);
    text_box_init(document, "interp", 15, 15, cilist[0]);
    nextcol(document);
    document.write("Possible:");
    nextcol(document);
    updboxsel(document, "interp", cilist);

    tabrowleft_cs(document, true, 3);
    radio_button(document, "state", "canc", "Create job in cancelled state", true);

    tabrowleft_cs(document, true, 3);
    radio_button(document, "state", "runnable", "Create job ready to run", false);
    endrow(document);
    if  (!fromfile)  {
	tabrowleft_cs(document, false, 3);
	checkbox_item_chk(document, "addnl", "Add newline to last line of job if it doesn't have it", true);
	endrow(document);
    }
}

function gbj_crvalidate(fromfile)  {
    if  (isblank(document.js_func_form.directory))  {
	alert("Must have a working directory");
	return  false;
    }
    if  (fromfile)  {
	if  (isblank(document.js_func_form.jobfile))  {
	    alert("Must have a file");
	    return  false;
	}
    }
    else  {
	var jd = document.js_func_form.jobdata.value;
	if  (document.js_func_form.addnl.checked  &&  jd.substr(jd.length-1) != "\n")
	    document.js_func_form.jobdata.value += "\n";
    }
    return  true;
}

//========================================================================
//========================================================================
//
//	Batch - variable hacking
//
//------------------------------------------------------------------------
//------------------------------------------------------------------------

function gbv_create()  {
    var w = initformwnd("gbvc", "Create Xi-Batch Variable", "#DFDFFF", "Create Variable", 460, 230, 5000);
    var d = w.document;
    d.writeln("<form name=js_func_form method=get onSubmit=\"return gbv_create2('",
	      makenewurl("btvccgi"), "');\">\n<table>");
    tabrowleft(d, false);
    d.write("Variable name");
    nextcol(d);
    text_box(d, "vname", 19, 19);
    tabrowleft(d, true);
    d.writeln("Initial value");
    nextcol(d);
    text_box(d, "vvalue", 19, 49);
    endtable(d);
    action_buttons(d, "Create Variable");
    d.writeln("</form>\n</body>");
}

function gbv_create2(url) {
    var dcf = document.js_func_form;
    if  (isblank(dcf.vname))  {
	alert("No variable name");
	dcf.focus();
	return  false;
    }
    if  (!vnameok(dcf.vname, true))  {
	alert("Invalid variable name");
	dcf.focus();
	return  false;
    }
    document.location = url + '&create&' + escape(dcf.vname.value) + '&' + escape(dcf.vvalue.value);
    return  false;
}

function getselvar(s, permbit)  {
    var varlist = listclicked();
    if  (varlist.length == 0)
	return  false;
    if  (varlist.length > 1)  {
	alert("Sorry you can only " + s + " one variable at a time");
	return  false;
    }
    var varname = varlist[0].value;
    var ind = varname.indexOf(',');
    if  (ind <= 0)
	return  varname;
    var num = parseInt(varname.substr(ind+1));
    varname = varname.substr(0, ind);
    if  (num & permbit)
	return  varname;
    alert("Sorry you cannot " + s + " " + varname);
    return  false;
}

function asswin(varname, nam, descr, op, wid, mxwid)
{
    var w = initformwnd(nam, descr + " Xi-Batch Variable " + varname, "#DFDFFF", descr + " Variable " + varname, 460, 180, 5000);
    var d = w.document;
    d.writeln("<form name=assform method=get onSubmit=\"return gbv_assrest('",
	      makenewurl("btvccgi"), "&", op, "&", escape(varname), "');\">\n<table>");
    tabrowleft(d, false);
    d.writeln(descr, " value");
    nextcol(d);
    text_box(d, "svalue", wid, mxwid);
    endtable(d);
    action_buttons(d, descr, " Variable");
    d.writeln("</form>\n</body>");
    return  false;
}

function gbv_assrest(urlcode)  {
    document.location = urlcode + '&' + escape(document.assform.svalue.value);
    return  false;
}

function gbv_assign()  {
    var varname = getselvar("assign to", 8);
    if  (varname)
	    asswin(varname, "gbva", "Assign", "assign", 19, 49);
}

function gbv_comment()  {
    var varname = getselvar("change comment of", 8);
    if  (varname)
	    asswin(varname, "gbvc", "Change comment", "comment", 19, 41);
}

function gbv_incrdecr(idec)  {
    var varname = getselvar("increment or decrement", 8);
    if  (varname)
	    vreswin(makenewurl("btvccgi") + "&" + (idec > 0? "incr" : "decr") + "&" + escape(varname) + "&1", 460, 230);
}

function gbv_delete()  {
    var varname = getselvar("Delete", 4);
    if  (varname)
	    vreswin(makenewurl("btvccgi") + "&delete&" + escape(varname), 460, 230);
}

function gbv_network()  {
    var varname = getselvar("set export flags", 4);
    if  (!varname)
	return;
    var w = initformwnd("varec", "Export markers for Xi-Batch Variable " + varname, "#DFDFFF",
			"Export/Cluster Variable " + varname, 460, 240, 5000);
    var d = w.document;
    d.writeln("<form name=expform method=get onSubmit=\"return gbv_exprest('",
	      makenewurl("btvccgi"), "&export&", escape(varname), "');\">\n<table>");
    tabrowleft(d, false);
    radio_button(d, "export", "l", "Local to server only", true);
    tabrowleft(d, true);
    radio_button(d, "export", "e", "Exported - visible on other servers", false);
    tabrowleft(d, true);
    radio_button(d, "export", "c", "Clustered - use local copy on executing server", false);
    endtable(d);
    action_buttons(d, "Set export variable");
    d.writeln("</form>\n</body>");
}

function gbv_exprest(urlcode)  {
    var  els = document.expform.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var  tag = els[cnt];
	if  (tag.type == "radio"  &&  tag.checked)  {
	    document.location = urlcode + '&' + escape(tag.value);
	    return  false;
	}
    }
    window.close();
    return  false;
}	

function gbv_perms()  {
    var varname = getselvar("set permissions", 0x20);
    if  (varname)
	    document.location = urlandfirstarg(document.location) + '&listperms' + escape(':' + varname);
}

function gbv_perms2()  {
    var clist = listchanged(document.js_func_form);
    var sets = "", unsets = "";
    for  (var el in clist)
	if  (clist[el])
	    sets += el;
        else
	    unsets += el;
    if  (sets.length == 0  &&  unsets.length == 0)
	return  gb_cancperm();
    if  (sets.length == 0)
	sets = "-";
    if  (unsets.length == 0)
	unsets = "-";
    var newloc = makenewurl("btvccgi") + "&perms&" + escape(document.js_func_form.vname.value) + "&" + sets + "&" + unsets;
    timer_setup(open(newloc, "pres", "width=460,height=230"), true, 3000);
    return  false;
}

function gbv_joblist()  {
    top.Butts.location = "/gbjobbutts.html";
    document.location = urldir(document.location) + remote_vn(document.location, "btjcgi");
}

function gbj_viewvars()  {
    top.Butts.location = "/gbvarbutts.html";
    document.location = urldir(document.location) + remote_vn(document.location, "btvcgi");
}

// Local Variables:
// mode: java
// cperl-indent-level: 4
// End:
