Name:       prepatch

Summary:    A patchmanager alternative for Sailfish OS which doesn't modify any files on disk.
Version:    0.2.1
Release:    1
Group:      Qt/Qt
License:    Other
Source0:    %{name}-%{version}.tar.bz2
Requires:   patch

%description
A patchmanager alternative for Sailfish OS which doesn't modify any files on disk.

%prep
%setup -q -n %{name}-%{version}

%build
gcc -O3 -Wall -ldl -fPIC -shared -o build/libprepatch.so src/prepatch.c


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/prepatch
mkdir -p %{buildroot}/usr/lib
mkdir -p %{buildroot}/etc
cp build/libprepatch.so %{buildroot}/usr/lib/libprepatch.so
echo /usr/lib/libprepatch.so > %{buildroot}/etc/ld.so.preload


%pre

%preun

%files
%defattr(-,root,root,-)
/usr/lib/
/etc/
