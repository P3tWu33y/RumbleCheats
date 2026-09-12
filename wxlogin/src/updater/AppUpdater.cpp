#include "AppUpdater.h"
#define NOMINMAX

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include "nlohmann/json.hpp"

#pragma comment(lib, "winhttp.lib")

namespace
{
	using json = nlohmann::json;

	std::wstring Utf8ToWide(const std::string& value)
	{
		if (value.empty())
			return {};

		const int size = MultiByteToWideChar(
			CP_UTF8,
			0,
			value.data(),
			static_cast<int>(value.size()),
			nullptr,
			0
		);

		if (size <= 0)
			return {};

		std::wstring result(size, L'\0');

		MultiByteToWideChar(
			CP_UTF8,
			0,
			value.data(),
			static_cast<int>(value.size()),
			result.data(),
			size
		);

		return result;
	}

	std::string GetLastErrorString()
	{
		const DWORD error = GetLastError();

		if (error == ERROR_SUCCESS)
			return {};

		return "WinHTTP error: " + std::to_string(error);
	}
}

AppUpdater::AppUpdater(std::string databaseUrl)
	: m_databaseUrl(std::move(databaseUrl))
{
	while (!m_databaseUrl.empty() && m_databaseUrl.back() == '/')
		m_databaseUrl.pop_back();
}

AppUpdater::Result AppUpdater::Request(const std::string& url)
{
	Result result;

	URL_COMPONENTS components{};
	components.dwStructSize = sizeof(components);

	wchar_t hostName[256]{};
	wchar_t urlPath[4096]{};
	wchar_t extraInfo[4096]{};

	components.lpszHostName = hostName;
	components.dwHostNameLength =
		static_cast<DWORD>(std::size(hostName));

	components.lpszUrlPath = urlPath;
	components.dwUrlPathLength =
		static_cast<DWORD>(std::size(urlPath));

	components.lpszExtraInfo = extraInfo;
	components.dwExtraInfoLength =
		static_cast<DWORD>(std::size(extraInfo));

	const std::wstring wideUrl = Utf8ToWide(url);

	if (!WinHttpCrackUrl(
		wideUrl.c_str(),
		static_cast<DWORD>(wideUrl.size()),
		0,
		&components))
	{
		result.error = GetLastErrorString();
		return result;
	}

	HINTERNET session = WinHttpOpen(
		L"AppUpdater/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (!session)
	{
		result.error = GetLastErrorString();
		return result;
	}

	DWORD secureProtocols =
		WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
		WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

	WinHttpSetOption(
		session,
		WINHTTP_OPTION_SECURE_PROTOCOLS,
		&secureProtocols,
		sizeof(secureProtocols)
	);

	// Downloads can be large -- give this more room than a JSON call.
	WinHttpSetTimeouts(
		session,
		5000,
		5000,
		15000,
		60000
	);

	HINTERNET connection = WinHttpConnect(
		session,
		components.lpszHostName,
		components.nPort,
		0
	);

	if (!connection)
	{
		result.error = GetLastErrorString();
		WinHttpCloseHandle(session);
		return result;
	}

	std::wstring requestPath = components.lpszUrlPath;

	if (components.lpszExtraInfo &&
		components.dwExtraInfoLength > 0)
	{
		requestPath += components.lpszExtraInfo;
	}

	DWORD flags = 0;

	if (components.nScheme == INTERNET_SCHEME_HTTPS)
		flags |= WINHTTP_FLAG_SECURE;

	HINTERNET request = WinHttpOpenRequest(
		connection,
		L"GET",
		requestPath.c_str(),
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		flags
	);

	if (!request)
	{
		result.error = GetLastErrorString();

		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);

		return result;
	}

	// GitHub Releases responds with a redirect to the actual asset URL --
	// WinHTTP follows redirects automatically by default, so no extra
	// handling is needed here.

	if (!WinHttpSendRequest(
		request,
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		WINHTTP_NO_REQUEST_DATA,
		0,
		0,
		0))
	{
		result.error = GetLastErrorString();

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);

		return result;
	}

	if (!WinHttpReceiveResponse(
		request,
		nullptr))
	{
		result.error = GetLastErrorString();

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);

		return result;
	}

	DWORD statusCode = 0;
	DWORD statusSize = sizeof(statusCode);

	if (!WinHttpQueryHeaders(
		request,
		WINHTTP_QUERY_STATUS_CODE |
		WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX,
		&statusCode,
		&statusSize,
		WINHTTP_NO_HEADER_INDEX))
	{
		result.error = GetLastErrorString();

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);

		return result;
	}

	result.httpStatus = static_cast<long>(statusCode);

	std::string response;

	for (;;)
	{
		DWORD available = 0;

		if (!WinHttpQueryDataAvailable(
			request,
			&available))
		{
			result.error = GetLastErrorString();
			break;
		}

		if (available == 0)
			break;

		std::vector<char> buffer(available);
		DWORD downloaded = 0;

		if (!WinHttpReadData(
			request,
			buffer.data(),
			available,
			&downloaded))
		{
			result.error = GetLastErrorString();
			break;
		}

		response.append(buffer.data(), downloaded);
	}

	result.rawResponse = std::move(response);

	result.success =
		result.error.empty() &&
		statusCode >= 200 &&
		statusCode < 300;

	if (!result.success &&
		result.error.empty())
	{
		result.error =
			"AppUpdater HTTP error " +
			std::to_string(statusCode);
	}

	WinHttpCloseHandle(request);
	WinHttpCloseHandle(connection);
	WinHttpCloseHandle(session);

	return result;
}

AppUpdater::VersionInfo AppUpdater::CheckForUpdate()
{
	VersionInfo info;

	const std::string url =
		m_databaseUrl + "/AppVersion.json";

	const Result result = Request(url);

	if (!result.success)
	{
		info.error =
			result.error.empty()
			? "Failed to check for updates."
			: result.error;

		return info;
	}

	try
	{
		const json value = json::parse(result.rawResponse);

		info.latestVersion = value.value("Latest", "");
		info.downloadUrl = value.value("DownloadUrl", "");
		info.forceUpdate = value.value("ForceUpdate", false);

		info.success =
			!info.latestVersion.empty() &&
			!info.downloadUrl.empty();

		if (!info.success)
			info.error = "Version info response was incomplete.";
	}
	catch (const std::exception& e)
	{
		info.error =
			std::string("Invalid version info response: ") +
			e.what();
	}

	return info;
}

bool AppUpdater::IsNewerVersion(
	const std::string& local,
	const std::string& remote)
{
	auto parseParts = [](const std::string& value)
		{
			std::vector<int> parts;
			std::stringstream stream(value);
			std::string segment;

			while (std::getline(stream, segment, '.'))
			{
				try
				{
					parts.push_back(std::stoi(segment));
				}
				catch (const std::exception&)
				{
					parts.push_back(0);
				}
			}

			return parts;
		};

	const std::vector<int> localParts = parseParts(local);
	const std::vector<int> remoteParts = parseParts(remote);

	const size_t count =
		std::max(localParts.size(), remoteParts.size());

	for (size_t i = 0; i < count; ++i)
	{
		const int localValue =
			(i < localParts.size()) ? localParts[i] : 0;

		const int remoteValue =
			(i < remoteParts.size()) ? remoteParts[i] : 0;

		if (remoteValue != localValue)
			return true;
	}

	return false;
}

bool AppUpdater::DownloadFile(
	const std::string& url,
	const std::wstring& destinationPath)
{
	const Result result = Request(url);

	if (!result.success)
		return false;

	std::ofstream out(destinationPath, std::ios::binary);

	if (!out.is_open())
		return false;

	out.write(
		result.rawResponse.data(),
		static_cast<std::streamsize>(result.rawResponse.size())
	);

	return out.good();
}

bool AppUpdater::LaunchUpdaterAndExit(
	const std::wstring& downloadedExePath,
	const std::wstring& targetExePath)
{
	wchar_t currentExePath[MAX_PATH]{};
	GetModuleFileNameW(nullptr, currentExePath, MAX_PATH);

	std::wstring updaterPath = currentExePath;
	const size_t lastSlash = updaterPath.find_last_of(L"\\/");

	if (lastSlash != std::wstring::npos)
		updaterPath = updaterPath.substr(0, lastSlash + 1);

	updaterPath += L"updater.exe";

	std::wstring commandLine =
		L"\"" + updaterPath + L"\" " +
		L"\"" + downloadedExePath + L"\" " +
		L"\"" + targetExePath + L"\" " +
		std::to_wstring(GetCurrentProcessId());

	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION processInfo{};

	const BOOL created = CreateProcessW(
		nullptr,
		commandLine.data(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		nullptr,
		&startupInfo,
		&processInfo
	);

	if (!created)
		return false;

	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	return true;
}

std::vector<std::uint8_t> AppUpdater::DownloadToMemory(const std::string& url, std::string* error)
{
	const Result result = Request(url);

	if (!result.success)
	{
		if (error)
			*error = result.error;

		return {};
	}

	const auto* begin =
		reinterpret_cast<const std::uint8_t*>(result.rawResponse.data());

	const auto* end =
		begin + result.rawResponse.size();

	return std::vector<std::uint8_t>(begin, end);
}