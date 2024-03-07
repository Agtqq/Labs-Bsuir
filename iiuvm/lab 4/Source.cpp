#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <devguid.h>
#include <setupapi.h>
#include <iostream>

#pragma comment(lib, "setupapi.lib")

using namespace std;
using namespace cv;

//cap - ����� � ���-������
// ������� ������ ����� � ���-������
void record(VideoCapture& cap, VideoWriter& video, bool isShowed, int duration) {
    int counter = 0;
    Mat frame;
    chrono::steady_clock::time_point start, end; //������ � ����� ��������
    start = chrono::steady_clock::now();
    // ������ ����� � ������� ��������� �����������������
    while (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() < duration * 1000) {
        cap >> frame; //���������� ����� �� �����������
        counter++;
        video.write(frame); // ����, ���������� �� �����������, ����������� � ���������, ������� ��������� � �������� ������
        char c = waitKey(1);
        // ����������� ����� �� ������
        if (isShowed)
            imshow("Video", frame);
        if (c == 27) // ����� �� ����� ��� ������� ESC
            break;
    }
    end = chrono::steady_clock::now();
    double seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000;
    system("cls");
    cout << "Camera FPS: " << trunc(counter / seconds) << endl; // ����� FPS ������
}

// ������� ��� ��������� ���������� � ������
void camInfo() {
    // ��������� ���������� � ������������ �������
    HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_CAMERA, "USB", NULL, DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE)
        return;

    SP_DEVINFO_DATA devInfoData;
    WCHAR buffer[512];
    char instanceIDBuffer[1024];

    for (int i = 0;; i++) {
        devInfoData.cbSize = sizeof(devInfoData); // ��������� ������� ��������� devInfoData

        // ���� ��� ���������, ��������� ����
        if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData) == FALSE)
            break;

        memset(buffer, 0, sizeof(buffer)); // ��������� ������
        // ��������� �������� ������� ���������� (������������� ���) � ���������� ��� � ������
        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (BYTE*)buffer, 512, NULL);

        wstring name, ids;
        // ������ �������� �� ������ � ������ name
        for (int i = 0; i < 512; i++)
            name += buffer[i];

        memset(buffer, 0, sizeof(buffer)); // ��������� ������
        // ��������� �������� ������� ���������� (������������� ������������) � ���������� ��� � ������
        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (BYTE*)buffer, 512, NULL);
        // ������ �������� �� ������ � ������ ids
        for (int i = 0; i < 512; i++)
            ids += buffer[i];
        // ��������� ��������� � ��������������� ������������� (Vendor ID) � ���������� (Device ID)
        wstring ven(ids.substr(ids.find(L"VID_") + 4, 4)); // VID - Vendor ID
        wstring dev(ids.substr(ids.find(L"PID_") + 4, 4)); // PID - Product ID

        memset(buffer, 0, sizeof(buffer)); // ��������� ������
        // ��������� �������������� ���������� ���������� � ���������� ��� � ������
        SetupDiGetDeviceInstanceIdA(devInfo, &devInfoData, (PSTR)instanceIDBuffer, 512, NULL);
        string instanceID(instanceIDBuffer); // ����������� ������ � ������

        // ���� ��������� ������� ����� ��������� � ��������������� ����������, ������� ��
        if (name.substr(name.size() - 4, 4) == dev)
            name = name.substr(0, name.size() - 7);

        // ����� ���������� � ������
        std::cout << "Information about camera:" << endl;
        wcout << L"Name: " << name << endl;
        wcout << L"Vendor ID: " << ven << endl;
        wcout << L"Device ID: " << dev << endl;
        cout << "Instance ID: " << instanceID << endl;

        // �������� ���������� �� ����������
        SetupDiDeleteDeviceInfo(devInfo, &devInfoData);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
}

// �������� ������� ���������
int main() {
    // ������������� ���� ������� � ���������
    HWND hConsole = GetConsoleWindow();
    ShowWindow(hConsole, SW_SHOW); //����������� ��� ������� ���� �� ������
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // �������� ����������� � ������
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "Camera is not open";
        return 0;
    }
    cap.set(CAP_PROP_FRAME_HEIGHT, INT_MAX);
    cap.set(CAP_PROP_FRAME_WIDTH, INT_MAX);
    Size size = { (int)cap.get(3), (int)cap.get(4) };

    // �������� ������� ��� ������ �����
    VideoWriter video("face.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 15, size); //������ VideoWriter ������������ ��� ������ ����� � ����
    Mat frame;
    system("cls");
    // ����� ���������� � ������
    camInfo();
    cout << "Maximal resolution: " << size.width << " x " << size.height << endl;
    cout << "1. Video recording\n2. Stealth video recording\n3. Photo\nchoice: ";
    int choice = 0;

    while (true) {
        cin >> choice;

        switch (choice) {
        default:
            cout << "Invalid choice. Please enter a valid choice." << endl;
            break;
        case 1:
            // ������ ����� � ������������ ����������� �� ������
            record(cap, video, true, 900);
            video.release(); // ���������� ������ �����
            break;

        case 2:
            // ������� ������ ����� ��� ����������� ����������� �� ������
            ShowWindow(hConsole, SW_HIDE);
            record(cap, video, false, 10);
            Sleep(2000); // ����� � 2 �������
            ShowWindow(hConsole, SW_SHOW);
            break;
        case 3:
            // ��������� � ���������� ����������
            cout << "Taking a photo..." << endl;
            cap >> frame;
            cap >> frame; // ������ ����� �� �����������
            if (!frame.empty()) {
                imshow("Image", frame); //����������
                imwrite("Image.jpg", frame); //���������
                cout << "Photo saved as Image.jpg" << endl;
            }
            else {
                cout << "Failed to capture an image." << endl;
            }
            waitKey(0);
            destroyWindow("Image");
            break;
        }

        if (choice == 1 || choice == 2) {
            char c = waitKey();
            if (c == 27) {
                video.release(); // ���������� ������ ����� �� ������� ESC
                break;
            }
        }
    }
    destroyAllWindows();
    //ShowWindow(hConsole, SW_SHOW);
    video.release();
    cap.release();
}
