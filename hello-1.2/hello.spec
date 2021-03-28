name:           hello
version:        1.0.0
release:        1%{?dst}
summary:        hello sdc
license:        -
vendor:         huawei

%define __strip %{cross}-strip

%prep

%build

%install
mkdir -p %{buildroot}
mkdir -p %{buildroot}/cfg
mkdir -p %{buildroot}/bin
mkdir -p %{buildroot}/lib

cd -

install -m 755 ./bin/main %{buildroot}/bin/main
install -m 755 ./config.* %{buildroot}/
install -m 755 ./*.png %{buildroot}/
install -m 755 ./portal.conf %{buildroot}/
install -m 755 ./sdc.conf %{buildroot}/
 
%clean
rm -rf %{buildroot}

%files
/*

%changelog

%description
example app on sdc os 


