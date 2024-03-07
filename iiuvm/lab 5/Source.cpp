#include <windows.h>
#include <dbt.h>
#include <strsafe.h>
#include <WinUser.h>
#include <iostream>
#include <map>
#include <initguid.h>
#include <Usbiodef.h>
#include <iostream>
#include <SetupAPI.h>
#include <vector>
#include <Cfgmgr32.h>
#include <conio.h>
#include <processthreadsapi.h>

#pragma comment(lib, "user32.lib" )
#pragma comment (lib, "Setupapi.lib")

using namespace std;

#define CLS_NAME "DUMMY_CLASS"

struct UsbDeviceDesc
{
	DEVINST devInst;
	string friendlyName;
	wstring name;
	HANDLE handle;
	bool safety;
	bool removable;
};

vector<UsbDeviceDesc> deviceArray;


//сравнение строк
bool caseUnsensCmp(wstring s1, wstring s2)
{
	if (s1.size() != s2.size())
		return false;
	for (int i = 0; i < s1.size(); i++)
		if (toupper(s1[i]) != toupper(s2[i]))
			return false;
	return true;
}

//возвращает читаемое имя устройства (friendlyName) по его обработчику
string handleToFriendlyName(HANDLE handle)
{
	for (auto it : deviceArray)
		if (it.handle == handle)
			return it.friendlyName;
}

//позволяет проверить безопасность устройства, исходя из его имен
bool nameToSafety(wstring name)
{
	for (auto it : deviceArray)
		if (caseUnsensCmp(it.name, name))
			return it.safety;
	return false;
}

// вашем коде устанавливает значение безопасности
void setSafety(HANDLE handle, bool safety)
{
	for (int i = 0; i < deviceArray.size(); i++)
		if (deviceArray[i].handle == handle)
			deviceArray[i].safety = safety;
}


void deleteByName(wstring name)
{
	for (int i = 0; i < deviceArray.size(); i++)
		if (caseUnsensCmp(deviceArray[i].name, name))
		{
			deviceArray.erase(deviceArray.begin() + i);
			return;
		}
}

string getFriendlyName(wchar_t* name)
{
	// Создание списка информации об устройствах
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);

	// Открытие интерфейса устройства в списке
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);

	// Инициализация структуры информации об устройстве
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	// Перечисление информации об устройствах в списке (получение информации для первого устройства)
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);

	// Получение размера необходимого буфера для свойства реестра устройства
	DWORD size = 0;
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_DEVICEDESC, NULL, NULL, NULL, &size);

	// Создание буфера нужного размера для получения свойства реестра устройства
	BYTE* buffer = new BYTE[size];
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_DEVICEDESC, NULL, buffer, size, NULL);

	// Преобразование полученных байтов в строку
	string deviceDesc = (char*)buffer;

	// Освобождение памяти, выделенной для буфера
	delete[] buffer;

	// Возврат читаемого имени устройства
	return deviceDesc;
}



bool getRemoveability(wchar_t* name)
{
	// Создание списка информации об устройствах
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);

	// Открытие интерфейса устройства в списке
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);

	// Инициализация структуры информации об устройстве
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	// Перечисление информации об устройствах в списке (получение информации для первого устройства)
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);

	// Получение свойства реестра устройства, отвечающего за его возможность быть удаленным (removable)
	DWORD props;
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_CAPABILITIES, NULL, (PBYTE)&props, sizeof(DWORD), NULL);

	// Возврат значения, указывающего, является ли устройство съемным (removable)
	return props & CM_DEVCAP_REMOVABLE;
}


string getFriendlyName(PDEV_BROADCAST_DEVICEINTERFACE_A info)
{
	wchar_t* name = (wchar_t*)info->dbcc_name;
	return getFriendlyName(name);
}

string getInstId(const wchar_t* name)
{
	// Создание списка информации об устройствах
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);

	// Открытие интерфейса устройства в списке по его имени
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);

	// Инициализация структуры информации об устройстве
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	// Перечисление информации об устройствах в списке (получение информации для первого устройства)
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);

	// Буфер для хранения Instance ID устройства
	BYTE buffer[BUFSIZ];

	// Получение Instance ID устройства и запись его в буфер
	SetupDiGetDeviceInstanceIdA(deviceList, &deviceInfo, (PSTR)buffer, BUFSIZ, NULL);

	// Преобразование байтов в строку и возврат Instance ID устройства
	return (char*)buffer;
}


LRESULT FAR PASCAL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Проверяем, является ли сообщение WM_DEVICECHANGE
	if (message == WM_DEVICECHANGE)
	{
		// Приводим lParam к типу PDEV_BROADCAST_HDR
		PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;

		// Проверяем, что устройство только что подключено
		if (wParam == DBT_DEVICEARRIVAL)
		{
			// Проверяем тип устройства (интерфейс устройства)
			if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				// Получаем информацию о подключенном устройстве и выводим сообщение
				PDEV_BROADCAST_DEVICEINTERFACE_A info = (PDEV_BROADCAST_DEVICEINTERFACE_A)lpdb;
				cout << "Device \"" << getFriendlyName(info) << "\" has been connected!\n";

				// Создаем обработчик для устройства и получаем уникальный идентификатор (Instance ID) устройства
				HANDLE deviceHandle = CreateFileW((wchar_t*)info->dbcc_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				DEV_BROADCAST_HANDLE deviceFilter;
				deviceFilter.dbch_size = sizeof(deviceFilter);
				deviceFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
				deviceFilter.dbch_handle = deviceHandle;
				HDEVNOTIFY notifyHandle = RegisterDeviceNotificationW(hWnd, &deviceFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
				CloseHandle(deviceHandle);

				DEVINST devInst;
				CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)getInstId((wchar_t*)info->dbcc_name).c_str(), CM_LOCATE_DEVNODE_NORMAL);

				// Создаем структуру для описания подключенного USB-устройства и добавляем ее в массив устройств
				UsbDeviceDesc tempDesc;
				tempDesc.devInst = devInst;
				tempDesc.handle = deviceHandle;
				tempDesc.name = (wchar_t*)info->dbcc_name;
				tempDesc.safety = false;
				tempDesc.friendlyName = getFriendlyName(info);
				tempDesc.removable = getRemoveability((wchar_t*)info->dbcc_name);
				deviceArray.push_back(tempDesc);
			}

			// Если тип устройства - том (например, флеш-накопитель), выводим информацию о логических дисках
			if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME info = (PDEV_BROADCAST_VOLUME)lpdb;
				if (info->dbcv_unitmask)
				{
					cout << "Logical drives on this device: ";
					for (int i = 0; i < 31; i++)
						if (info->dbcv_unitmask & (DWORD)pow(2, i))
							cout << (char)('A' + i) << ' ';
					cout << '\n';
				}
			}
		}

		// Устройство подвергается запросу на удаление
		if (wParam == DBT_DEVICEQUERYREMOVE)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				// Устанавливаем безопасность устройства в "true"
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				setSafety(info->dbch_handle, true);
			}
		}

		// Запрос на удаление устройства был неудачным
		if (wParam == DBT_DEVICEQUERYREMOVEFAILED)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				// Выводим сообщение о неудачной попытке безопасного удаления устройства
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				cout << "Failed to safely remove device \"" << handleToFriendlyName(info->dbch_handle) << "\"\n";

				// Устанавливаем безопасность устройства в "false"
				setSafety(info->dbch_handle, false);
			}
		}

		// Устройство было удалено
		if (wParam == DBT_DEVICEREMOVECOMPLETE)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				// Получаем информацию об удаленном устройстве и выводим сообщение
				PDEV_BROADCAST_DEVICEINTERFACE_A info = (PDEV_BROADCAST_DEVICEINTERFACE_A)lpdb;
				cout << "Device \"" << getFriendlyName(info) << "\" has been removed"
					<< (nameToSafety((wchar_t*)info->dbcc_name) ? " safely!" : "!") << '\n';

				// Удаляем информацию об устройстве из массива устройств
				deleteByName((wchar_t*)info->dbcc_name);
			}

			// Отменяем регистрацию уведомлений об удалении для данного устройства
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				UnregisterDeviceNotification(info->dbch_hdevnotify);
			}
		}
	}

	// Возвращаемся к стандартной обработке сообщений окна
	return DefWindowProc(hWnd, message, wParam, lParam);
}

DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter)
{
	// Инициализация переменных
	HWND hWnd = NULL;
	WNDCLASSEX wx;
	ZeroMemory(&wx, sizeof(wx));

	// Настройка параметров окна
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = (WNDPROC)(WndProc); // Указатель на обработчик окна
	wx.lpszClassName = (LPCWSTR)CLS_NAME; // Имя класса окна

	// Создание GUID для устройства USB
	GUID guid = GUID_DEVINTERFACE_USB_DEVICE;

	// Регистрация класса окна
	if (RegisterClassEx(&wx)) // Если регистрация прошла успешно
		hWnd = CreateWindowW((LPCWSTR)CLS_NAME, (LPCWSTR)("DevNotifWnd"), WS_ICONIC, 0, 0, CW_USEDEFAULT, 0, 0, NULL, GetModuleHandle(0), (void*)&guid);

	// Настройка параметров фильтра уведомлений об устройствах
	DEV_BROADCAST_DEVICEINTERFACE_A filter;
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_classguid = guid;
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	RegisterDeviceNotificationW(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

	// Получение информации об устройствах USB
	HDEVINFO devicesHandle = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	DWORD deviceNumber = 0;
	SP_DEVICE_INTERFACE_DATA devinterfaceData;
	devinterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	// Перечисление и обработка устройств
	while (SetupDiEnumDeviceInterfaces(devicesHandle, NULL, &GUID_DEVINTERFACE_USB_DEVICE, deviceNumber++, &devinterfaceData))
	{
		// Получение информации об интерфейсе устройства
		DWORD bufSize = 0;
		SetupDiGetDeviceInterfaceDetailW(devicesHandle, &devinterfaceData, NULL, NULL, &bufSize, NULL);
		BYTE* buffer = new BYTE[bufSize];
		PSP_DEVICE_INTERFACE_DETAIL_DATA_W devinterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buffer;
		devinterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		SetupDiGetDeviceInterfaceDetailW(devicesHandle, &devinterfaceData, devinterfaceDetailData, bufSize, NULL, NULL);

		// Получение имени устройства
		wchar_t* name = devinterfaceDetailData->DevicePath;

		// Получение Instance ID и создание дескриптора устройства
		DEVINST devInst;
		CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)getInstId(devinterfaceDetailData->DevicePath).c_str(), CM_LOCATE_DEVNODE_NORMAL);
		HANDLE deviceHandle = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		// Настройка фильтра уведомлений для данного устройства
		DEV_BROADCAST_HANDLE deviceFilter;
		deviceFilter.dbch_size = sizeof(deviceFilter);
		deviceFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
		deviceFilter.dbch_handle = deviceHandle;
		HDEVNOTIFY notifyHandle = RegisterDeviceNotificationW(hWnd, &deviceFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		CloseHandle(deviceHandle);

		// Создание и добавление информации об устройстве в массив
		UsbDeviceDesc tmpUsbDev;
		tmpUsbDev.devInst = devInst;
		tmpUsbDev.friendlyName = getFriendlyName(name);
		tmpUsbDev.handle = deviceHandle;
		tmpUsbDev.name = name;
		tmpUsbDev.safety = false;
		tmpUsbDev.removable = getRemoveability(name);
		deviceArray.push_back(tmpUsbDev);
	}

	// Переменная-флаг для управления циклом обработки сообщений
	bool flag = true;

	// Цикл обработки сообщений
	MSG msg;
	while (GetMessageW(&msg, hWnd, 0, 0) || _kbhit())
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Возвращение результата
	return 0;
}


int main()
{
	setlocale(LC_ALL, "Russian");
	system("chcp 1251");
	system("cls");
	CreateThread(NULL, NULL, ThreadProc, NULL, NULL, NULL);
	while (1)
	{
		int id = 1;
		int choise = 0;
		cin >> choise;
		if (!choise)
		{
			cout << "      List of connected USB devices" << endl;
			for (auto it : deviceArray)
				cout << id++ << " - " << it.friendlyName << (it.removable ? " [REMOVABLE]" : "") << endl;
			cout << endl << "If you want to remove the device enter its number " << endl;
			cout << endl << "If you want to update the list of connected USB devices enter 0 " << endl;
			continue;
		}
		if (deviceArray[choise - 1].removable)
			CM_Request_Device_EjectW(deviceArray[choise - 1].devInst, NULL, NULL, NULL, NULL);
		else
			cout << "This device is NOT removable!\n";
	}
	return 0;
}