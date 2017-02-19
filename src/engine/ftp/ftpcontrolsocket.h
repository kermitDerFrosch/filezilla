#ifndef FILEZILLA_ENGINE_FTP_FTPCONTROLSOCKET_HEADER
#define FILEZILLA_ENGINE_FTP_FTPCONTROLSOCKET_HEADER

#include "logging_private.h"
#include "ControlSocket.h"
#include "externalipresolver.h"
#include "rtt.h"

#include <regex>

namespace PrivCommand {
auto const cwd = Command::private1;
auto const rawtransfer = Command::private2;
}

#define RECVBUFFERSIZE 4096
#define MAXLINELEN 2000

class CTransferSocket;
class CFtpTransferOpData;
class CFtpRawTransferOpData;
class CTlsSocket;

class CFtpControlSocket final : public CRealControlSocket
{
	friend class CTransferSocket;
public:
	CFtpControlSocket(CFileZillaEnginePrivate & engine);
	virtual ~CFtpControlSocket();
	virtual void TransferEnd();

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);

protected:

	virtual int ResetOperation(int nErrorCode);

	// Implicit FZ_REPLY_CONTINUE
	virtual void Connect(CServer const& server) override;
	virtual void List(CServerPath const& path = CServerPath(), std::wstring const& subDir = std::wstring(), int flags = 0) override;
	void ChangeDir(CServerPath const& path = CServerPath(), std::wstring const& subDir = std::wstring(), bool link_discovery = false);
	virtual void FileTransfer(std::wstring const& localFile, CServerPath const& remotePath,
							 std::wstring const& remoteFile, bool download,
							 CFileTransferCommand::t_transferSettings const& transferSettings) override;
	virtual void RawCommand(std::wstring const& command) override;
	virtual int Delete(CServerPath const& path, std::deque<std::wstring>&& files) override;
	virtual int RemoveDir(CServerPath const& path, std::wstring const& subDir) override;
	virtual int Mkdir(CServerPath const& path) override;
	virtual int Rename(CRenameCommand const& command) override;
	virtual int Chmod(CChmodCommand const& command) override;
	void Transfer(std::wstring const& cmd, CFtpTransferOpData* oldData);


	virtual void OnConnect();
	virtual void OnReceive();

	int SendCommand(std::wstring const& str, bool maskArgs = false, bool measureRTT = true);

	// Parse the latest reply line from the server
	void ParseLine(std::wstring line);

	// Parse the actual response and delegate it to the handlers.
	// It's the last line in a multi-line response.
	void ParseResponse();

	virtual bool CanSendNextCommand() const override;

	int GetReplyCode() const;

	int GetExternalIPAddress(std::string& address);

	void StartKeepaliveTimer();

	std::wstring m_Response;
	std::wstring m_MultilineResponseCode;
	std::vector<std::wstring> m_MultilineResponseLines;

	std::unique_ptr<CTransferSocket> m_pTransferSocket;

	// Some servers keep track of the offset specified by REST between sessions
	// So we always sent a REST 0 for a normal transfer following a restarted one
	bool m_sentRestartOffset{};

	char m_receiveBuffer[RECVBUFFERSIZE];
	int m_bufferLen{};
	int m_repliesToSkip{}; // Set to the amount of pending replies if cancelling an action

	int m_pendingReplies{1};

	std::unique_ptr<CExternalIPResolver> m_pIPResolver;

	CTlsSocket* m_pTlsSocket{};
	bool m_protectDataChannel{};

	int m_lastTypeBinary{-1};

	// Used by keepalive code so that we're not using keep alive
	// till the end of time. Stop after a couple of minutes.
	fz::monotonic_clock m_lastCommandCompletionTime;

	fz::timer_id m_idleTimer{};

	CLatencyMeasurement m_rtt;

	virtual void operator()(fz::event_base const& ev);

	void OnExternalIPAddress();
	void OnTimer(fz::timer_id id);

	std::unique_ptr<std::wregex> m_pasvReplyRegex; // Have it as class member to avoid recompiling the regex on each transfer or listing

	friend class CProtocolOpData<CFtpControlSocket>;
	friend class CFtpChangeDirOpData;
	friend class CFtpChmodOpData;
	friend class CFtpDeleteOpData;
	friend class CFtpFileTransferOpData;
	friend class CFtpListOpData;
	friend class CFtpLogonOpData;
	friend class CFtpMkdirOpData;
	friend class CFtpRawCommandOpData;
	friend class CFtpRawTransferOpData;
	friend class CFtpRemoveDirOpData;
	friend class CFtpRenameOpData;
};

typedef CProtocolOpData<CFtpControlSocket> CFtpOpData;

class CFtpTransferOpData
{
public:
	CFtpTransferOpData() = default;
	virtual ~CFtpTransferOpData() = default;

	TransferEndReason transferEndReason{TransferEndReason::successful};
	bool tranferCommandSent{};

	int64_t resumeOffset{};
	bool binary{true};
};

#endif
