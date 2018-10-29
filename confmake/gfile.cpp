/*
** Author: Samuel R. Blackburn
**
** $Workfile: GFile.cpp $
** $Revision: 13 $
** $Modtime: 2/21/01 5:02a $
** $Reuse Tracing Code: 1001 $
*/

#include <stdafx.h>
#include <assert.h>
#include <crtdbg.h>
#include "gfile.h"
#include <tchar.h>

#pragma hdrstop
#pragma warning( disable : 4311 )

// Helper functions
BOOL PASCAL wfc_close_handle( HANDLE handle )
{
	BOOL return_value = FALSE;

	try
	{
		// CloseHandle will throw an exception.... DOH!
		return_value = CloseHandle( handle );

		if ( return_value == FALSE )
		{
			return( FALSE );
		}
		else
		{
			return_value = TRUE;
		}
	}
	catch( ... )
	{
	}

	return( return_value );
}

static inline bool __is_directory_separation_character( TCHAR temp_pointer )
{
	return( temp_pointer == '\\' || temp_pointer == '/' );
}

static inline UINT __GetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
	//WFC_VALIDATE_POINTER( lpszPathName );
	//WFC_VALIDATE_POINTER( lpszTitle );

	// always capture the complete file name including extension (if present)
	LPTSTR lpszTemp = (LPTSTR)lpszPathName;

	LPCTSTR temp_pointer = NULL;

	for ( temp_pointer = lpszPathName; *temp_pointer != '\0'; temp_pointer = _tcsinc( temp_pointer ) )
	{
		// remember last directory/drive separator
		if (*temp_pointer == '\\' || *temp_pointer == '/' || *temp_pointer == ':')
		{
			lpszTemp = (LPTSTR) _tcsinc( temp_pointer );
		}
	}

	// lpszTitle can be NULL which just returns the number of bytes
	if (lpszTitle == NULL)
	{
		return( lstrlen( lpszTemp ) + 1 );
	}

	// otherwise copy it into the buffer provided
	lstrcpyn( lpszTitle, lpszTemp, nMax );

	return( 0 );
}

static inline void __GetRoot( LPCTSTR path_name, std::string& root_string )
{
	//WFC_VALIDATE_POINTER( path_name );

	TCHAR root_name[ _MAX_PATH ];

	LPTSTR root_pointer = root_name;

	ZeroMemory( root_pointer, _MAX_PATH );

	lstrcpyn( root_pointer, path_name, _MAX_PATH );

	LPTSTR temp_pointer = NULL;

	for ( temp_pointer = root_name; *temp_pointer != '\0'; temp_pointer = _tcsinc( temp_pointer ) )
	{
		// find first double slash and stop
		if ( __is_directory_separation_character( temp_pointer[ 0 ] ) &&
			  __is_directory_separation_character( temp_pointer[ 1 ] ) )
		{
			break;
		}
	}

	if ( *temp_pointer != '\0' )
	{
		// it is a UNC name, find second slash past '\\'
		assert( __is_directory_separation_character( temp_pointer[ 0 ] ) );
		assert( __is_directory_separation_character( temp_pointer[ 1 ] ) );

		temp_pointer += 2;

		while ( *temp_pointer != '\0' && ( ! __is_directory_separation_character( *temp_pointer ) ) )
		{
			temp_pointer = _tcsinc( temp_pointer );
		}

		if ( *temp_pointer != '\0' )
		{
			temp_pointer = _tcsinc( temp_pointer );
		}

		while ( *temp_pointer != '\0' && ( ! __is_directory_separation_character( *temp_pointer ) ) )
		{
			temp_pointer = _tcsinc( temp_pointer );
		}

		// terminate it just after the UNC root (ie. '\\server\share\')

		if ( *temp_pointer != '\0' )
		{
			temp_pointer[ 1 ] = '\0';
		}
	}
	else
	{
		// not a UNC, look for just the first slash
		temp_pointer = root_pointer;

		while ( *temp_pointer != '\0' && ( ! __is_directory_separation_character( *temp_pointer ) ) )
		{
			temp_pointer = _tcsinc( temp_pointer );
		}

		// terminate it just after root (ie. 'x:\')
		if ( *temp_pointer != '\0' )
		{
			temp_pointer[ 1 ] = '\0';
		}
	}

	root_string = root_name;
}

BOOL __FullPath( LPTSTR path_out, LPCTSTR file_in )
{
	//WFC_VALIDATE_POINTER( path_out );
	//WFC_VALIDATE_POINTER( file_in );

	// first, fully qualify the path name
	LPTSTR file_part = NULL;

	if ( ! GetFullPathName( file_in, _MAX_PATH, path_out, &file_part ) )
	{
		lstrcpyn( path_out, file_in, _MAX_PATH ); // take it literally
		return( FALSE );
	}

	std::string strRoot;

	// determine the root name of the volume
	__GetRoot(path_out, strRoot);

	// get file system information for the volume

	DWORD dwFlags = 0;
	DWORD dwDummy = 0;

	if ( ! GetVolumeInformation(strRoot.c_str(), NULL, 0, NULL, &dwDummy, &dwFlags, NULL, 0 ) )
	{
		return( FALSE );	// preserving case may not be correct
	}

	// not all characters have complete uppercase/lowercase

	if ( ! ( dwFlags & FS_CASE_IS_PRESERVED ) )
	{
		CharUpper( path_out );
	}

	// assume non-UNICODE file systems, use OEM character set

	if ( ! ( dwFlags & FS_UNICODE_STORED_ON_DISK ) )
	{
		WIN32_FIND_DATA data;

		HANDLE h = FindFirstFile( file_in, &data );

		if ( h != (HANDLE) INVALID_HANDLE_VALUE )
		{
			FindClose( h );
			lstrcpy( file_part, data.cFileName );
		}
	}

	return( TRUE );
}

UINT __GetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
	//WFC_VALIDATE_POINTER( lpszPathName );
	//WFC_VALIDATE_POINTER( lpszTitle );

	// use a temporary to avoid bugs in ::GetFileTitle when lpszTitle is NULL
	TCHAR temp_string[ _MAX_PATH ];

	LPTSTR lpszTemp = lpszTitle;

	if (lpszTemp == NULL)
	{
		lpszTemp = temp_string;
		nMax = _countof( temp_string );
	}

	// 2000-05-23
	// There is a huge performance penality for calling GetFileTitle().
	// You should avoid it at all costs.

	if ( ::GetFileTitle( lpszPathName, lpszTemp, (WORD) nMax ) != 0 )
	{
		// when ::GetFileTitle fails, use cheap imitation
		return ( ::__GetFileName( lpszPathName, lpszTitle, nMax ) );
	}

	return lpszTitle == NULL ? ::lstrlen( lpszTemp ) + 1 : 0;
}

GFile::GFile()
{
	//TRACE( TEXT( "GFile::GFile()" ) );
	//WFC_VALIDATE_POINTER( this );

	m_FileHandle			  = (HANDLE) INVALID_HANDLE_VALUE;
	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

	m_Initialize();
}

GFile::GFile( __int64 file_handle )
{
	//TRACE( TEXT( "GFile::GFile( int )" ) );
	//WFC_VALIDATE_POINTER( this );

	m_FileHandle			  = (HANDLE) INVALID_HANDLE_VALUE;
	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

	m_Initialize();

	m_hFile			= (HANDLE) file_handle; // Stupid public member that is never used internally
	m_FileHandle	 = (HANDLE) file_handle;
	m_CloseOnDelete = FALSE;
}

GFile::GFile( LPCTSTR filename, UINT open_flags )
{
	//TRACE( TEXT( "GFile::GFile( LPCTSTR )" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER( filename );

	m_FileHandle			  = (HANDLE) INVALID_HANDLE_VALUE;
	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

	m_Initialize();

	if ( Open( filename, open_flags ) == FALSE )
	{
		//AfxThrowFileException( exception.m_cause, exception.m_lOsError, exception.m_strFileName );
	}
}

GFile::~GFile()
{
	//TRACE( TEXT( "GFile::~GFile()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( m_FileHandle != (HANDLE) INVALID_HANDLE_VALUE && m_CloseOnDelete != FALSE )
	{
		Close();
	}

	m_Uninitialize();

	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;
}

void GFile::Abort( void )
{
	//TRACE( TEXT( "GFile::Abort()" ) );
	//WFC_VALIDATE_POINTER( this );

	BOOL return_value = TRUE;

	if ( m_FileHandle != (HANDLE) INVALID_HANDLE_VALUE )
	{
		if (wfc_close_handle( m_FileHandle ) != 0 )
		{
			return_value = FALSE;
		}

		m_FileHandle = (HANDLE) INVALID_HANDLE_VALUE;
	}

	m_FileName.clear();
	m_PathName.clear();
	m_FileTitle.clear();
	m_CloseOnDelete = FALSE;

	// Invalidate that stupid public attribute

	m_hFile = (HANDLE) hFileNull;
}

void GFile::Close( void )
{
	//TRACE( TEXT( "GFile::Close()" ) );
	//WFC_VALIDATE_POINTER( this );

	BOOL return_value = TRUE;

	if ( m_FileHandle != (HANDLE) INVALID_HANDLE_VALUE )
	{
		if (wfc_close_handle( m_FileHandle ) != 0 )
		{
			return_value = FALSE;
		}

		m_FileHandle = (HANDLE) INVALID_HANDLE_VALUE;
	}

	m_FileName.clear();
	m_PathName.clear();
	m_FileTitle.clear();
	m_CloseOnDelete = FALSE;

	// Invalidate that stupid public attribute

	m_hFile = (HANDLE) hFileNull;
}

GFile * GFile::Duplicate( void ) const
{
	//TRACE( TEXT( "GFile::Duplicate()" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_COVERAGE( 2 );

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		return( (GFile *) NULL );
	}

	GFile * return_value = NULL;
	
	try
	{
		return_value = new GFile;
	}
	catch( ... )
	{
		return( NULL );
	}

	HANDLE duplicate_file_handle = (HANDLE) INVALID_HANDLE_VALUE;

	if ( ::DuplicateHandle( ::GetCurrentProcess(), m_FileHandle, ::GetCurrentProcess(), &duplicate_file_handle, 0, FALSE, DUPLICATE_SAME_ACCESS ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );

		delete return_value;
		return_value = NULL;
	}
	else
	{
		return_value->m_hFile			= (HANDLE) duplicate_file_handle; // Stupid public attribute that will never be used internally
		return_value->m_FileHandle	 = duplicate_file_handle;
		return_value->m_CloseOnDelete = m_CloseOnDelete;
		return_value->m_PathName		= m_PathName;
		return_value->m_FileName		= m_FileName;
		return_value->m_FileTitle	  = m_FileTitle;
	}

	return( return_value );
}

void GFile::Flush( void )
{
	//TRACE( TEXT( "GFile::Flush()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		return;
	}

	if ( ::FlushFileBuffers( m_FileHandle ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
	}
}

std::string GFile::GetFileName( void ) const
{
	//TRACE( TEXT( "GFile::GetFileName()" ) );
	//WFC_VALIDATE_POINTER( this );

	return( m_FileName );
}

std::string GFile::GetFilePath( void ) const
{
	//TRACE( TEXT( "GFile::GetFilePath()" ) );
	//WFC_VALIDATE_POINTER( this );

	return( m_PathName );
}

std::string GFile::GetFileTitle( void ) const
{
	//TRACE( TEXT( "GFile::GetFileTitle()" ) );
	//WFC_VALIDATE_POINTER( this );

	// 2000-05-23
	// You pay an enourmous performance penalty for calling GetFilePath().
	// The children at Microsoft actually create a COM object and
	// read tons of registry settings when you call GetFilePath().
	// This is not a good thing when you are trying to be fast.
	// We will delay the penality as long as possible.

	if ( m_FileTitle.size() == 0 &&
		  m_PathName.size() > 0 )
	{
		TCHAR file_title[ _MAX_FNAME ];

		if ( ::__GetFileTitle( m_PathName.c_str(), file_title, _MAX_FNAME ) == 0 )
		{
			const_cast< std::string& >( m_FileTitle ) = file_title;
		}
		else
		{
			const_cast< std::string& >( m_FileTitle ).clear();
		}
	}

	return( m_FileTitle );
}

HANDLE GFile::GetHandle( void ) const
{
	//TRACE( TEXT( "GFile::GetHandle()" ) );
	//WFC_VALIDATE_POINTER( this );
	return( m_FileHandle );
}

BOOL GFile::GetInformation( BY_HANDLE_FILE_INFORMATION& information ) const
{
	//TRACE( TEXT( "GFile::GetInformation()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( ::GetFileInformationByHandle( m_FileHandle, &information ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
		return( FALSE );
	}

	return( TRUE );
}

ULONGLONG GFile::GetLength( void ) const
{
	//TRACE( TEXT( "GFile::GetLength()" ) );
	//WFC_VALIDATE_POINTER( this );

	LARGE_INTEGER length;

	length.QuadPart = 0;

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		return( length.QuadPart );
	}

	length.LowPart = ::GetFileSize( m_FileHandle, (DWORD *) &length.HighPart );

	if ( length.LowPart == 0xFFFFFFFF && ( ::GetLastError() != NO_ERROR ) )
	{
		//WFCTRACEERROR( ::GetLastError() );
		length.QuadPart = 0;
	}

	return( length.QuadPart );
}

ULONGLONG GFile::GetPosition( void ) const
{
	//TRACE( TEXT( "GFile::GetHandle()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		//WFCTRACE( TEXT( "File is not open." ) );
		return( (ULONGLONG) -1 );
	}

	LARGE_INTEGER return_value;
	LARGE_INTEGER zero = { 0, 0 };

	return_value.QuadPart = 0;

	// There's a bug in NT where SetFilePointer() doesn't set last error correctly or at all...

	if ( ::SetFilePointerEx( m_FileHandle, zero, &return_value, FILE_CURRENT ) == 0 )
	{
		// An Error Occurred
		//WFCTRACEERROR( ::GetLastError() );
		return_value.QuadPart = 0;
	}

	return( return_value.QuadPart );
}

SECURITY_ATTRIBUTES * GFile::GetSecurityAttributes( void ) const
{
	//TRACE( TEXT( "GFile::GetSecurityAttributes()" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_COVERAGE( 11 );
	return( m_SecurityAttributes_p );
}

SECURITY_DESCRIPTOR * GFile::GetSecurityDescriptor( void ) const
{
	//TRACE( TEXT( "GFile::GetSecurityDescriptor()" ) );
	//WFC_VALIDATE_POINTER( this );
	return( m_SecurityDescriptor_p );
}

#if 0

BOOL GFile::GetStatus( GFileStatus& status ) const
{
	WFC_VALIDATE_POINTER( this );
	ZeroMemory( &status, sizeof( status ) );

	BY_HANDLE_FILE_INFORMATION information;

	if ( GetInformation( information ) == FALSE )
	{
		return( FALSE );
	}

	_tcsncpy( status.m_szFullName, m_FileName, DIMENSION_OF( status.m_szFullName ) );

	if ( information.nFileSizeHigh != 0 )
	{
		// File is too large to return information about
		return( FALSE );
	}

	if ( m_FileName.IsEmpty() )
	{
		status.m_attribute = 0;
	}
	else
	{
		status.m_attribute = (BYTE) information.dwFileAttributes;

		// convert times as appropriate
		status.m_ctime = CTime( information.ftCreationTime	);
		status.m_atime = CTime( information.ftLastAccessTime );
		status.m_mtime = CTime( information.ftLastWriteTime  );

		if ( status.m_ctime.GetTime() == 0 )
		{
			status.m_ctime = status.m_mtime;
		}

		if ( status.m_atime.GetTime() == 0 )
		{
			status.m_atime = status.m_mtime;
		}
	}

	return( TRUE );
}

BOOL PASCAL GFile::GetStatus( LPCTSTR filename, GFileStatus& status )
{
	WFC_VALIDATE_POINTER( this );
	WFC_VALIDATE_POINTER( filename );

	if ( ::__FullPath( status.m_szFullName, filename ) == FALSE )
	{
		status.m_szFullName[ 0 ] = TEXT( '\0' );
		return( FALSE );
	}

	WIN32_FIND_DATA find_data;

	HANDLE find_handle = ::FindFirstFile( (LPTSTR) filename, &find_data );

	if ( find_handle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		return( FALSE );
	}

	FindClose( find_handle );

	status.m_attribute = (BYTE) ( find_data.dwFileAttributes & ~FILE_ATTRIBUTE_NORMAL );

	status.m_size = (LONG) find_data.nFileSizeLow;

	// convert times as appropriate
	status.m_ctime = CTime( find_data.ftCreationTime	);
	status.m_atime = CTime( find_data.ftLastAccessTime );
	status.m_mtime = CTime( find_data.ftLastWriteTime  );

	if ( status.m_ctime.GetTime() == 0 )
	{
		status.m_ctime = status.m_mtime;
	}

	if ( status.m_atime.GetTime() == 0 )
	{
		status.m_atime = status.m_mtime;
	}

	return( TRUE );
}

#endif

void GFile::m_Initialize( void )
{
	//TRACE( TEXT( "GFile::m_Initialize()" ) );
	//WFC_VALIDATE_POINTER( this );

	m_CloseOnDelete = FALSE;

	try
	{
		m_SecurityAttributes_p = new SECURITY_ATTRIBUTES;
	}
	catch( ... )
	{
		m_SecurityAttributes_p = NULL;
	}
	

	if ( m_SecurityAttributes_p == NULL )
	{
		return;
	}

	try
	{
		m_SecurityDescriptor_p = new SECURITY_DESCRIPTOR;
	}
	catch( ... )
	{
		m_SecurityDescriptor_p = NULL;
	}
	

	if ( m_SecurityDescriptor_p == NULL )
	{
		delete m_SecurityAttributes_p;
		m_SecurityAttributes_p = NULL;

		return;
	}

	if ( ::InitializeSecurityDescriptor( m_SecurityDescriptor_p, SECURITY_DESCRIPTOR_REVISION ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
		//WFCTRACE( TEXT( "InitializeSecurityDescriptor() failed with the above error." ) );

		delete m_SecurityAttributes_p;
		m_SecurityAttributes_p = NULL;

		delete m_SecurityDescriptor_p;
		m_SecurityDescriptor_p = NULL;

		return;
	}

	if ( ::SetSecurityDescriptorDacl( m_SecurityDescriptor_p, TRUE, NULL, FALSE ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
		//WFCTRACE( TEXT( "SetSecurityDescriptorDacl() failed with the above error." ) );

		delete m_SecurityAttributes_p;
		m_SecurityAttributes_p = NULL;

		delete m_SecurityDescriptor_p;
		m_SecurityDescriptor_p = NULL;

		return;
	}

	m_SecurityAttributes_p->nLength				  = sizeof( SECURITY_ATTRIBUTES );
	m_SecurityAttributes_p->lpSecurityDescriptor = m_SecurityDescriptor_p;
	m_SecurityAttributes_p->bInheritHandle		 = TRUE;
}

void GFile::m_Uninitialize( void )
{
	//TRACE( TEXT( "GFile::m_Uninitialize()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( m_SecurityAttributes_p != NULL )
	{
		delete m_SecurityAttributes_p;
		m_SecurityAttributes_p = NULL;
	}

	if ( m_SecurityDescriptor_p != NULL )
	{
		delete m_SecurityDescriptor_p;
		m_SecurityDescriptor_p = NULL;
	}
}

void GFile::LockRange( ULONGLONG position, ULONGLONG number_of_bytes_to_lock )
{
	//TRACE( TEXT( "GFile::LockRange()" ) );
	//WFC_VALIDATE_POINTER( this );

	LARGE_INTEGER parameter_1;
	LARGE_INTEGER parameter_2;

	parameter_1.QuadPart = position;
	parameter_2.QuadPart = number_of_bytes_to_lock;

	if ( ::LockFile( m_FileHandle, parameter_1.LowPart, parameter_1.HighPart, parameter_2.LowPart, parameter_2.HighPart ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
		//GFileException::ThrowOsError( (LONG) ::GetLastError() );
	}
}

BOOL GFile::Open( LPCTSTR filename, UINT open_flags )
{
	//TRACE( TEXT( "GFile::Open()" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER( filename );

	Close();

	try
	{
		m_FileName = filename;

		TCHAR full_path[ _MAX_PATH ];

		::__FullPath( full_path, filename );

		m_PathName = full_path;

		m_FileTitle.clear();

		open_flags &= ~ (UINT) typeBinary;

		DWORD access = 0;

		switch ( open_flags & 3 )
		{
			case modeRead:

				access = GENERIC_READ;
				break;

			case modeWrite:

				access = GENERIC_WRITE;
				break;

			case modeReadWrite:

				access = GENERIC_READ | GENERIC_WRITE;
				break;

			default:

				assert( FALSE );
		}

		DWORD share_mode = 0;

		switch ( open_flags & 0x70 )
		{
			case shareCompat:
			case shareExclusive:

				share_mode = 0;
				break;

			case shareDenyWrite:

				share_mode = FILE_SHARE_READ;
				break;

			case shareDenyRead:

				share_mode = FILE_SHARE_WRITE;
				break;

			case shareDenyNone:

				share_mode = FILE_SHARE_WRITE | FILE_SHARE_READ;
				break;

			default:

				assert( FALSE );
		}

		if ( m_SecurityAttributes_p != NULL )
		{
			m_SecurityAttributes_p->bInheritHandle = ( ( open_flags & modeNoInherit ) == 0 ) ? TRUE : FALSE;
		}

		DWORD creation_flags = 0;

		if ( open_flags & modeCreate )
		{
			if ( open_flags & modeNoTruncate )
			{
				creation_flags = OPEN_ALWAYS;
			}
			else
			{
				creation_flags = CREATE_ALWAYS;
			}
		}
		else
		{
			creation_flags = OPEN_EXISTING;
		}

		// 2001-02-21
		// Thanks go to Roman Mak (roman@matrix.spb.ru) for finding a bug here.
		// I was applying the flags to the create_flags parameter and not the
		// attributes parameter.

		DWORD attributes = FILE_ATTRIBUTE_NORMAL;

		if ( open_flags & osNoBuffer )
		{
			attributes |= FILE_FLAG_NO_BUFFERING;
		}

		if ( open_flags & osRandomAccess )
		{
			attributes |= FILE_FLAG_RANDOM_ACCESS;
		}

		if ( open_flags & osSequentialScan )
		{
			attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
		}

		if ( open_flags & osWriteThrough )
		{
			attributes |= FILE_FLAG_WRITE_THROUGH;
		}

		if ( open_flags & wfcDeleteOnClose )
		{
			attributes |= FILE_FLAG_DELETE_ON_CLOSE;
		}

		m_FileHandle = ::CreateFile( filename,
											  access,
											  share_mode,
											  GetSecurityAttributes(),
											  creation_flags,
											  attributes,
											  NULL );

		if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
		{
			//WFCTRACEERROR( ::GetLastError() );

			Close();
			return( FALSE );
		}

		m_hFile = (HANDLE) m_FileHandle; // Set the stupid public member that is never used internally

		m_CloseOnDelete = TRUE;

		return( TRUE );
	}
	catch( ... )
	{
		return( FALSE );
	}
	
}

//DWORD GFile::Read( CByteArray& buffer, DWORD number_of_bytes_to_read )
//{
//	TRACE( TEXT( "GFile::Read( CByteArray )" ) );
//	WFC_VALIDATE_POINTER( this );
//
//	buffer.SetSize( number_of_bytes_to_read, 8192 );
//
//	return( Read( (void *) buffer.GetData(), buffer.GetSize() ) );
//}

UINT GFile::Read( void * buffer, UINT number_of_bytes_to_read )
{
	//TRACE( TEXT( "GFile::Read( void * )" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER_NULL_OK( buffer );
	////WFCTRACEVAL( TEXT( "Reading " ), number_of_bytes_to_read );

#if defined( _DEBUG )
	if ( number_of_bytes_to_read == 1 )
	{
		//WFC_COVERAGE( 29 );
	}
#endif // _DEBUG

	if ( number_of_bytes_to_read == 0 )
	{
		return( 0 );
	}

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		return( 0 );
	}

	DWORD number_of_bytes_read = 0;

	if ( ::ReadFile( m_FileHandle, buffer, number_of_bytes_to_read, &number_of_bytes_read, NULL ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
		//WFCTRACE( TEXT( "Can't read from file because of above error." ) );
	}

	return( number_of_bytes_read );
}

DWORD GFile::ReadHuge( void * buffer, DWORD number_of_bytes_to_read )
{
	//WFC_VALIDATE_POINTER( this );
	return( Read( buffer, number_of_bytes_to_read ) );
}

void PASCAL GFile::Rename( LPCTSTR old_name, LPCTSTR new_name )
{
	//WFC_VALIDATE_POINTER( old_name );
	//WFC_VALIDATE_POINTER( new_name );

	if ( ::MoveFile( (LPTSTR) old_name, (LPTSTR) new_name ) == FALSE )
	{
	}
}

void PASCAL GFile::Remove( LPCTSTR filename )
{
	//WFC_VALIDATE_POINTER( filename );

	if ( ::DeleteFile( (LPTSTR) filename ) == FALSE )
	{
	}
}

ULONGLONG GFile::Seek( ULONGLONG offset, UINT from )
{
	//TRACE( TEXT( "GFile::Seek()" ) );
	//WFC_VALIDATE_POINTER( this );

	if ( m_FileHandle == (HANDLE) INVALID_HANDLE_VALUE )
	{
		//WFCTRACE( TEXT( "File is not open." ) );
		return( (DWORD) -1 );
	}

	DWORD move_method = 0;

	switch( from )
	{
	  case GFile::begin:

		  ////WFCTRACEVAL( TEXT( "From beginning to " ), offset );
		  move_method = FILE_BEGIN;
		  break;

	  case GFile::current:

		  ////WFCTRACEVAL( TEXT( "From current to " ), offset );
		  move_method = FILE_CURRENT;
		  break;

	  case GFile::end:

		  ////WFCTRACEVAL( TEXT( "From end to " ), offset );
		  move_method = FILE_END;
		  break;

	  default:

		  ////WFCTRACEVAL( TEXT( "Unknown from position (it wasn't GFile::begin, GFile::current or GFile::end " ), from );
		  return( (DWORD) -1 );
	}

	LARGE_INTEGER return_value = { 0, 0 };
	LARGE_INTEGER distance_to_move;

	distance_to_move.QuadPart = offset;

	// There's a bug in NT where SetFilePointer() doesn't set last error correctly or at all...
	if (  ::SetFilePointerEx( m_FileHandle, distance_to_move, &return_value, move_method ) == 0 )
	{
		//WFCTRACEERROR( ::GetLastError() );
	}

	return( return_value.QuadPart );
}

void GFile::SeekToBegin( void )
{
	//TRACE( TEXT( "GFile::SeekToBegin()" ) );
	//WFC_VALIDATE_POINTER( this );

	Seek( (ULONGLONG) 0, GFile::begin );
}

ULONGLONG GFile::SeekToEnd( void )
{
	//TRACE( TEXT( "GFile::SeekToEnd()" ) );
	//WFC_VALIDATE_POINTER( this );
	return( Seek( 0, GFile::end ) );
}

BOOL GFile::SetEndOfFile( ULONGLONG length )
{
	//TRACE( TEXT( "GFile::SetEndOfFile()" ) );
	//WFC_VALIDATE_POINTER( this );

	Seek( length, GFile::begin );

	_ASSERTE( GetPosition() == length );

	if ( ::SetEndOfFile( m_FileHandle ) == FALSE )
	{
		////WFCTRACEERROR( ::GetLastError() );
		////WFCTRACE( TEXT( "Can't set end of file because of above error." ) );
		return( FALSE );
	}

	return( TRUE );
}

void GFile::SetFilePath( LPCTSTR new_name )
{
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER( new_name );
	m_FileName = new_name;
}

void GFile::SetLength( ULONGLONG new_length )
{
	//WFC_VALIDATE_POINTER( this );

	if ( SetEndOfFile( new_length ) == FALSE )
	{
	}
}

#if 0
void PASCAL GFile::SetStatus( LPCTSTR filename, const GFileStatus& status )
{
	GFile::SetStatus( filename, status );
}
#endif

void GFile::UnlockRange( ULONGLONG position, ULONGLONG number_of_bytes_to_unlock )
{
	//TRACE( TEXT( "GFile::UnlockRange()" ) );
	//WFC_VALIDATE_POINTER( this );

	LARGE_INTEGER parameter_1;
	LARGE_INTEGER parameter_2;

	parameter_1.QuadPart = position;
	parameter_2.QuadPart = number_of_bytes_to_unlock;

	if ( ::UnlockFile( m_FileHandle, parameter_1.LowPart, parameter_1.HighPart, parameter_2.LowPart, parameter_2.HighPart ) == FALSE )
	{
		////WFCTRACEERROR( ::GetLastError() );
	}
}

void GFile::Write( const void * buffer, UINT number_of_bytes_to_write )
{
	//TRACE( TEXT( "GFile::Write()" ) );
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER_NULL_OK( buffer );
	//WFC_COVERAGE( 26 );

	if ( number_of_bytes_to_write == 0 )
	{
		return;
	}

	DWORD number_of_bytes_written = 0;

	if ( ::WriteFile( m_FileHandle, buffer, number_of_bytes_to_write, &number_of_bytes_written, NULL ) == FALSE )
	{
		//WFCTRACEERROR( ::GetLastError() );
	}
}

void GFile::WriteHuge( const void * buffer, DWORD number_of_bytes_to_write )
{
	//WFC_VALIDATE_POINTER( this );
	//WFC_VALIDATE_POINTER_NULL_OK( buffer );
	Write( buffer, number_of_bytes_to_write );
}

// Operators

GFile::operator HFILE ( void ) const
{
	//WFC_VALIDATE_POINTER( this );
	return( (HFILE) m_FileHandle );
}
