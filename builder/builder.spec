Summary: system builder
Name: builder
Version: 1
Release: 0
License: MSH
Group: Applications/System
BuildArch: noarch
Provides: VMware::VIRuntime

%description
A utility to build systems

%files
%attr(644,root,root) /usr/local/lib/builder/vm_template.xml
%attr(644,root,root) /usr/local/lib/builder/poweron.xml
%attr(644,root,root) /usr/local/lib/builder/license.xml
%attr(644,root,root) /usr/local/lib/builder/power_saver.xml
%attr(644,root,root) /usr/local/lib/builder/mount.xml
%attr(644,root,root) /usr/local/lib/builder/ilo4_151.bin
%attr(644,root,root) /usr/local/lib/builder/psp.xml
%attr(644,root,root) /usr/local/lib/builder/updfw.xml
%attr(644,root,root) /usr/local/lib/builder/cold.xml
%attr(644,root,root) /usr/local/lib/builder/reset.xml
%attr(644,root,root) /usr/local/lib/builder/one_time.xml
%attr(644,root,root) /usr/local/lib/builder/ilo3_170.bin
%attr(644,root,root) /usr/local/lib/builder/ilo3_128.bin
%attr(644,root,root) /usr/local/lib/builder/ilo2_225.bin
%attr(755,root,root) /usr/local/lib/tools/replay.pl
%attr(755,root,root) /usr/local/lib/tools/locfg.pl
%attr(755,root,root) /usr/local/lib/tools/create_vms.pl
%attr(755,root,root) /usr/local/lib/tools/detect_ilo
%attr(755,root,root) /usr/local/lib/tools/check_ilo
%attr(755,root,root) /usr/local/lib/tools/getiso
%attr(755,root,root) /usr/local/bin/builder
