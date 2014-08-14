#!/usr/bin/perl

#$host = "meta-int.lback01g.tst.maps.yandex.ru";
#$host = "meta-int.deneb.maps.dev.yandex.net";
$host = "meta-int.lback01g.i18n.tst.maps.yandex.ru";
#$host = "pc.lback01g.tst.maps.yandex.ru";
if ( $host =~ /^(meta-int\.maps\.yandex\.net(:80)?|meta-int\.[a-z]+.?.?.?(\.i18n)?(\.tst|\.load)?\.maps\.yandex\.(ru|net)|meta-int\.[a-z]+.?.?.?\.maps\.dev\.yandex\.net)$/ ) { 
#if ( $host =~ /^pc([.\-a-z0-9]+)\.yandex\.(ru|net)$/ ) { 
    print "Match! \n";
} else { 
    print "No match! \n";
}
