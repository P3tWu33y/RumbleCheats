#include "ToolFrame.h"

#include "Theme.h"
#include "Config.h"
#include "AppUpdater.h"
#include "LoadLibraryR.h"
#include "LoginFrame.h"
#include "SharedParams.h"
#include "suspender.h"

namespace
{
	constexpr int kPollIntervalMs = 1;
	constexpr int kMaxDots = 3;
}

ToolFrame::ToolFrame(const wxString& loggedInAsUser)
	: wxFrame(nullptr, wxID_ANY, Config::kAppTitle,
		wxDefaultPosition, wxSize(460, 420),
		wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)),
	m_loggedInAsUser(loggedInAsUser)
{
	BuildUi();

	m_pollTimer.Bind(wxEVT_TIMER, &ToolFrame::OnPollTimer, this);
	m_pollTimer.Start(kPollIntervalMs);

	CentreOnScreen();
}

void ToolFrame::BuildUi()
{
	Theme::ApplyWindowTheme(this);

	wxPanel* root = new wxPanel(this);
	Theme::ApplyWindowTheme(root);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// --- Header -----------------------------------------------------
	wxStaticText* title = new wxStaticText(root, wxID_ANY, Config::kAppTitle);
	title->SetForegroundColour(Theme::TextPrimary);
	title->SetFont(Theme::TitleFont());

	wxString subtitleText = m_loggedInAsUser.IsEmpty()
		? wxString("Signed in")
		: wxString::Format("Signed in as %s", m_loggedInAsUser);

	wxStaticText* subtitle = new wxStaticText(root, wxID_ANY, subtitleText);
	subtitle->SetForegroundColour(Theme::TextMuted);
	subtitle->SetFont(Theme::SubtitleFont());

	rootSizer->Add(title, 0, wxLEFT | wxTOP | wxRIGHT, 24);
	rootSizer->Add(subtitle, 0, wxLEFT | wxRIGHT | wxBOTTOM, 24);

	// --- Card: waiting status ----------------------------------------
	wxPanel* card = new wxPanel(root);
	card->SetBackgroundColour(Theme::Card);

	wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

	m_statusLabel = new wxStaticText(card, wxID_ANY, "Waiting for RumbleFighter");
	m_statusLabel->SetForegroundColour(Theme::TextPrimary);
	m_statusLabel->SetFont(Theme::BodyFont().Bold());

	cardSizer->Add(m_statusLabel, 0, wxALIGN_CENTER | wxALL, 24);

	card->SetSizer(cardSizer);

	rootSizer->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
	rootSizer->AddStretchSpacer();

	// --- Footer ad -----------------------------------------------------
	wxStaticText* ad = Theme::CreateAdLabel(root, Config::kDiscordAd);

	rootSizer->Add(ad, 0, wxALIGN_CENTRE_HORIZONTAL | wxBOTTOM, 12);

	root->SetSizer(rootSizer);

	wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(root, 1, wxEXPAND);
	SetSizer(frameSizer);
}

void ToolFrame::OnPollTimer(wxTimerEvent& event)
{
	if (m_processFound)
		return;

	const auto processes = ProcessUtils::FindProcessesByName(Config::kTargetProcessName);

	if (!processes.empty())
	{
		m_processFound = true;
		m_pollTimer.Stop();

		m_statusLabel->SetLabel("RumbleFighter.exe found!");

		const DWORD pid = processes[0].pid;

		OnProcessFound(pid);

		return;
	}

	// Ping-pong the dot count: 0 -> 1 -> 2 -> 3 -> 2 -> 1 -> 0 -> ...
	m_dotCount += m_dotDirection;
	if (m_dotCount >= kMaxDots || m_dotCount <= 0)
		m_dotDirection = -m_dotDirection;

	m_statusLabel->SetLabel("Waiting for RumbleFighter" + wxString('.', m_dotCount));
}

void ToolFrame::OnProcessFound(DWORD pid)
{
	// RumbleFighter.exe has been detected.

	ThreadSuspender suspender;
	suspender.SuspendResumeProcess("RumbleFighter.exe", true);

	const std::string downloadUrl = "https://github.com/P3tWu33y/WxLogin-Releases/releases/download/" + Config::version + "/" + Config::assetName;

	AppUpdater updater(Config::kFirebaseDatabaseUrl);

	std::string error;

	const std::vector<std::uint8_t> binary =
		updater.DownloadToMemory(downloadUrl, &error);

	if (binary.empty())
	{
		//wxLogDebug("Download failed: %s", error);
		m_statusLabel->SetLabel("Download failed: " + error);
		Beep(500, 500);
		return;
	}

	// Validate that the downloaded data is a Windows PE file.
	if (binary.size() < 2 ||
		binary[0] != 0x4D ||
		binary[1] != 0x5A)
	{
		m_statusLabel->SetLabel(
			"Downloaded file is not a valid PE (missing MZ)"
		);
		Beep(500, 500);
		return;
	}


	m_statusLabel->SetLabel("Downloaded " + std::to_string(binary.size()) + " bytes");
	//wxLogDebug("Downloaded %zu bytes", binary.size());


	// binary.data() -> pointer to the bytes
	// binary.size() -> size of the binary

	//wxLogDebug(
	//	"Downloaded %zu bytes, first bytes: %02X %02X",
	//	binary.size(),
	//	binary.size() > 0 ? binary[0] : 0,
	//	binary.size() > 1 ? binary[1] : 0
	//);




	//LPVOID lpRemoteParam = NULL;
	HANDLE hProcess = NULL;
	HANDLE hToken = NULL;
	SIZE_T dwLength = binary.size();
	DWORD dwBytesRead = 0;
	DWORD dwProcessId = pid;
	DWORD dwExitCode = 1;
	TOKEN_PRIVILEGES priv = { 0 };


	/* --- Enable SeDebugPrivilege (as you had) --- */
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
			AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);
		CloseHandle(hToken);
	}

	/* --- Open target process --- */
	hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId);
	if (!hProcess)
	{
		//printf("[-] Failed to open target process %ls\n", targetExe);
		//system("pause");
		//Beep(500, 500);
		m_statusLabel->SetLabel("[-]Failed to open target process");
		return;
	}


	// --- Write parameters to target process memory --- */
	SharedParams params = {};
	std::string user = m_loggedInAsUser.ToStdString();

	// Cut @gmail.com
	auto at = user.find('@');
	if (at != std::string::npos)
		user = user.substr(0, at);

	strncpy_s(params.username, user.c_str(), _TRUNCATE);

	LPVOID pRemote = VirtualAllocEx(hProcess, NULL, sizeof(SharedParams),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	WriteProcessMemory(hProcess, pRemote, &params, sizeof(SharedParams), NULL);


	/* --- Inject reflectively from memory (bin) --- */
	HANDLE hModule = LoadRemoteLibraryR(hProcess, (LPVOID)binary.data(), (SIZE_T)dwLength, pRemote);

	if (!hModule)
	{
		//wxLogDebug("LoadRemoteLibraryR failed");
		m_statusLabel->SetLabel("[-]Error #1 failed");
		//Beep(500, 500);
		suspender.SuspendResumeProcess("RumbleFighter.exe", false);
		return;
	}
	else
	{
		//wxLogDebug("LoadRemoteLibraryR returned: %p", hModule);
		m_statusLabel->SetLabel("[+]Loaded Successfully!");
	}

	suspender.SuspendResumeProcess("RumbleFighter.exe", false);

	Close(true);
}