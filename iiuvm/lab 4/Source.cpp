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

//cap - поток с веб-камеры
// Функция записи видео с веб-камеры
void record(VideoCapture& cap, VideoWriter& video, bool isShowed, int duration) {
    int counter = 0;
    Mat frame;
    chrono::steady_clock::time_point start, end; //начало и конец отстчета
    start = chrono::steady_clock::now();
    // Запись видео в течение указанной продолжительности
    while (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() < duration * 1000) {
        cap >> frame; //извлечение кадра из видеопотока
        counter++;
        video.write(frame); // кадр, полученный из видеопотока, добавляется в видеофайл, который находится в процессе записи
        char c = waitKey(1);
        // Отображение видео на экране
        if (isShowed)
            imshow("Video", frame);
        if (c == 27) // Выход из цикла при нажатии ESC
            break;
    }
    end = chrono::steady_clock::now();
    double seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000;
    system("cls");
    cout << "Camera FPS: " << trunc(counter / seconds) << endl; // Вывод FPS камеры
}

// Функция для получения информации о камере
void camInfo() {
    // Получение информации о подключенных камерах
    HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_CAMERA, "USB", NULL, DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE)
        return;

    SP_DEVINFO_DATA devInfoData;
    WCHAR buffer[512];
    char instanceIDBuffer[1024];

    for (int i = 0;; i++) {
        devInfoData.cbSize = sizeof(devInfoData); // Установка размера структуры devInfoData

        // Если нет устройств, завершаем цикл
        if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData) == FALSE)
            break;

        memset(buffer, 0, sizeof(buffer)); // Обнуление буфера
        // Получение свойства реестра устройства (дружественное имя) и сохранение его в буфере
        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (BYTE*)buffer, 512, NULL);

        wstring name, ids;
        // Запись символов из буфера в строку name
        for (int i = 0; i < 512; i++)
            name += buffer[i];

        memset(buffer, 0, sizeof(buffer)); // Обнуление буфера
        // Получение свойства реестра устройства (идентификатор оборудования) и сохранение его в буфере
        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (BYTE*)buffer, 512, NULL);
        // Запись символов из буфера в строку ids
        for (int i = 0; i < 512; i++)
            ids += buffer[i];
        // Получение подстроки с идентификатором производителя (Vendor ID) и устройства (Device ID)
        wstring ven(ids.substr(ids.find(L"VID_") + 4, 4)); // VID - Vendor ID
        wstring dev(ids.substr(ids.find(L"PID_") + 4, 4)); // PID - Product ID

        memset(buffer, 0, sizeof(buffer)); // Обнуление буфера
        // Получение идентификатора экземпляра устройства и сохранение его в буфере
        SetupDiGetDeviceInstanceIdA(devInfo, &devInfoData, (PSTR)instanceIDBuffer, 512, NULL);
        string instanceID(instanceIDBuffer); // Конвертация буфера в строку

        // Если последние символы имени совпадают с идентификатором устройства, удаляем их
        if (name.substr(name.size() - 4, 4) == dev)
            name = name.substr(0, name.size() - 7);

        // Вывод информации о камере
        std::cout << "Information about camera:" << endl;
        wcout << L"Name: " << name << endl;
        wcout << L"Vendor ID: " << ven << endl;
        wcout << L"Device ID: " << dev << endl;
        cout << "Instance ID: " << instanceID << endl;

        // Удаление информации об устройстве
        SetupDiDeleteDeviceInfo(devInfo, &devInfoData);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
}

// Основная функция программы
int main() {
    // Инициализация окна консоли и кодировок
    HWND hConsole = GetConsoleWindow();
    ShowWindow(hConsole, SW_SHOW); //отображение или скрытие окон на экране
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Открытие видеопотока с камеры
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "Camera is not open";
        return 0;
    }
    cap.set(CAP_PROP_FRAME_HEIGHT, INT_MAX);
    cap.set(CAP_PROP_FRAME_WIDTH, INT_MAX);
    Size size = { (int)cap.get(3), (int)cap.get(4) };

    // Создание объекта для записи видео
    VideoWriter video("face.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 15, size); //объект VideoWriter используется для записи видео в файл
    Mat frame;
    system("cls");
    // Вывод информации о камере
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
            // Запись видео с отображением изображения на экране
            record(cap, video, true, 900);
            video.release(); // Завершение записи видео
            break;

        case 2:
            // Скрытая запись видео без отображения изображения на экране
            ShowWindow(hConsole, SW_HIDE);
            record(cap, video, false, 10);
            Sleep(2000); // Пауза в 2 секунды
            ShowWindow(hConsole, SW_SHOW);
            break;
        case 3:
            // Получение и сохранение фотографии
            cout << "Taking a photo..." << endl;
            cap >> frame;
            cap >> frame; // Захват кадра из видеопотока
            if (!frame.empty()) {
                imshow("Image", frame); //отображает
                imwrite("Image.jpg", frame); //сохраняет
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
                video.release(); // Завершение записи видео по нажатию ESC
                break;
            }
        }
    }
    destroyAllWindows();
    //ShowWindow(hConsole, SW_SHOW);
    video.release();
    cap.release();
}
