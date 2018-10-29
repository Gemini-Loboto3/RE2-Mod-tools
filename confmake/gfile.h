#pragma once
#include <string>

class GFile
{
private:
	GFile( const GFile& ) {};
	GFile& operator=( const GFile& ) { return( *this ); };

protected:
	SECURITY_ATTRIBUTES * m_SecurityAttributes_p;

	SECURITY_DESCRIPTOR * m_SecurityDescriptor_p;

	HANDLE m_FileHandle;

	std::string m_PathName;
	std::string m_FileName;
	std::string m_FileTitle;

	BOOL m_CloseOnDelete;

	void m_Initialize( void );
	void m_Uninitialize( void );

public:
	// Flag values
	enum OpenFlags
	{
		modeRead         = 0x0000,
		modeWrite        = 0x0001,
		modeReadWrite    = 0x0002,
		shareCompat      = 0x0000,
		shareExclusive   = 0x0010,
		shareDenyWrite   = 0x0020,
		shareDenyRead    = 0x0030,
		shareDenyNone    = 0x0040,
		modeNoInherit    = 0x0080,
		modeCreate       = 0x1000,
		modeNoTruncate   = 0x2000,
		typeText         = 0x4000,
		typeBinary       = 0x8000,
		osNoBuffer       = 0x10000,
		osWriteThrough   = 0x20000,
		osRandomAccess   = 0x40000,
		osSequentialScan = 0x80000,
		wfcDeleteOnClose = 0x100000,
	};

	enum Attribute
	{
		normal    = 0x00,
		readOnly  = 0x01,
		hidden    = 0x02,
		system    = 0x04,
		volume    = 0x08,
		directory = 0x10,
		archive   = 0x20,
	};

	enum SeekPosition
	{
		begin   = 0x0,
		current = 0x1,
		end     = 0x2,
	};

	enum
	{
		hFileNull = -1,
	};

	GFile();
	GFile( __int64 file_handle );
	GFile( LPCTSTR filename, UINT open_flags );
	~GFile();

	HANDLE m_hFile; // AGAINST WFC CODING STANDARDS!!!

	virtual operator HFILE() const;

	virtual void Abort( void );
	virtual void Close( void );
	virtual GFile * Duplicate( void ) const;
	virtual void Flush( void );
	virtual std::string GetFileName( void ) const;
	virtual std::string GetFilePath( void ) const;
	virtual std::string GetFileTitle( void ) const;
	virtual HANDLE GetHandle( void ) const;
	virtual BOOL GetInformation( BY_HANDLE_FILE_INFORMATION& information ) const;
	virtual ULONGLONG GetLength( void ) const;
	virtual ULONGLONG GetPosition( void ) const;
	virtual SECURITY_ATTRIBUTES * GetSecurityAttributes( void ) const;
	virtual SECURITY_DESCRIPTOR * GetSecurityDescriptor( void ) const;
	virtual void LockRange( ULONGLONG position, ULONGLONG number_of_bytes_to_lock );
	virtual BOOL Open( LPCTSTR filename, UINT open_flags );
	virtual UINT Read( void * buffer, UINT number_of_bytes_to_read );
	//virtual DWORD Read( CByteArray& buffer, DWORD number_of_bytes_to_read );
	virtual DWORD ReadHuge( void * buffer, DWORD number_of_bytes_to_read );
	virtual ULONGLONG Seek( ULONGLONG offset, UINT from );
	virtual void SeekToBegin( void );
	virtual ULONGLONG SeekToEnd( void );
	virtual BOOL SetEndOfFile( ULONGLONG length ); // when CBigFile becomes not read-only
	virtual void SetFilePath( LPCTSTR new_name );
	virtual void SetLength( ULONGLONG length );
	virtual void UnlockRange( ULONGLONG position, ULONGLONG number_of_bytes_to_unlock );
	virtual void Write( const void * buffer, UINT number_of_bytes_to_write );
	virtual void WriteHuge( const void * buffer, DWORD number_of_bytes_to_write );

	static void PASCAL Rename( LPCTSTR old_name, LPCTSTR new_name );
	static void PASCAL Remove( LPCTSTR filename );
};
