#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

// Подключаем WinSock, IP Helper API и ICMP API
#include <winsock2.h>  // WSADATA, WSAStartup, SOCKET
#include <iphlpapi.h>  // SendARP
#include <icmpapi.h>   // IcmpCreateFile, IcmpSendEcho
#include <stdio.h>
#include <iostream>
#include <cassert>

#pragma comment(lib, "Ws2_32.lib")    // Линкуем WinSock
#pragma comment(lib, "Iphlpapi.lib")  // Линкуем IP Helper API (включает IcmpSendEcho)

class Ping
{
public:
    Ping() = default;
    ~Ping() = default;

    void CheckIP(int argc);
    void ARPRequestToDetermineMAC(this Ping& object);
    void PrintMAC(this Ping& object);

    int OnPing(this Ping& object,int argc, char** argv);

private:
    ULONG m_MACAddr[2]{};

    // Ожидаем длину 6 байт
    ULONG m_physAddrLen{ 6 };

    unsigned long m_destIP{};

    DWORD m_ret{};
};

void Ping::CheckIP(int argc)
{
    // Проверяем, передан ли IP-адрес
    assert(argc == 2 && " IP address not entered! ");
}

void Ping::PrintMAC(this Ping& object)
{
    std::byte* bPhysAddr = reinterpret_cast<std::byte*>(&object.m_MACAddr);
    
    if (object.m_physAddrLen)
    {
        auto to_int = [](std::byte b) { return std::to_integer<int>(b); };
        
        std::cout << "MAC: ";
        for (int i = 0; i < static_cast<int>(object.m_physAddrLen); i++)
        {
            std::cout << std::hex << to_int(bPhysAddr[i]) << ((i != static_cast<int>(object.m_physAddrLen) - 1) ? ":" : "");
        }
        std::cout << std::endl;
    }
}

void Ping::ARPRequestToDetermineMAC(this Ping& object)
{
    memset(&object.m_MACAddr, 0xff, sizeof(object.m_MACAddr)); // Инициализируем

    object.m_ret = SendARP(object.m_destIP, 0, &object.m_MACAddr, &object.m_physAddrLen);

    assert(object.m_ret == NO_ERROR && " SendARP failed! ");
}

int Ping::OnPing(this Ping& object, int argc, char** argv)
{
    object.CheckIP(argc);
    
    const char* ip = argv[1];

    // Функция WSAStartup инициирует использование winsock DLL процессом.
    WSADATA wsaData{};

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        // Избавимся от предупреждения С6031
    };

    // Отправка ICMP Echo (Ping)
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE)
    {
        perror("IcmpCreateFile");
        return 1;
    }

    object.m_destIP = inet_addr(ip);

    // Полезная нагрузка
    char sendData[32] = "Data";

    // Буфер для ответа, включающий заголовок и данные
    char recvBuf[sizeof(ICMP_ECHO_REPLY) + sizeof(sendData)]{};

    // Отправляем и ждем до 1000 мс
    object.m_ret = IcmpSendEcho(hIcmp, object.m_destIP, sendData, sizeof(sendData), nullptr, recvBuf, sizeof(recvBuf), 1000);

    if (object.m_ret == 0)
    {
        std::cout << argv[0] << " Ping failed!" << std::endl;
        return 1;
    }

    // Обрабатываем ответ
    auto* reply = reinterpret_cast<ICMP_ECHO_REPLY*>(recvBuf);

    std::cout << "Reply from: " << ip << std::endl;
    std::cout << "RTT : " << reply->RoundTripTime << " ms" << std::endl;

    // Запрос ARP для определения MAC-адреса
    object.ARPRequestToDetermineMAC();
    

    // Выводим MAC-адрес
    object.PrintMAC();

    // Освобождаем ресурсы
    IcmpCloseHandle(hIcmp);
    WSACleanup();

    return 0;
}

int main(int argc, char* argv[]) {
    
    Ping p;
    p.OnPing(argc, argv);

    return 0;
}
