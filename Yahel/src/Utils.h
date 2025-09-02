#ifndef UTILS_H
#define UTILS_H

namespace Yahel{
namespace Utils{

	template<typename T>
	class CVarBackup{
		const T value0;
	protected:
		T &var;
	public:
		inline CVarBackup(T &var)
			: value0(var) , var(var) {
		}
		inline ~CVarBackup(){
			var=value0;
		}
		inline operator const T &() const{
			return value0;
		}
	};

	template<typename T>
	class CVarTempReset:public CVarBackup<T>{
	public:
		inline CVarTempReset(T &var,const T &newValue)
			: CVarBackup(var) {
			var=newValue;
		}
		inline operator T() const{ return var; }
	};

	class CExclusivelyLocked sealed{
		CRITICAL_SECTION &cs;
	public:
		CExclusivelyLocked(CRITICAL_SECTION &cs);
		~CExclusivelyLocked();
	};

	class CYahelPen:public LOGPEN{
		const HPEN handle;
	public:
		CYahelPen(BYTE thickness,COLORREF color);
		~CYahelPen();

		inline operator HPEN() const{ return handle; }
	};

	class CYahelBrush:public LOGBRUSH{
		HBRUSH handle;
	public:
		static const CYahelBrush None,Black,White;

		CYahelBrush(int stockObjectId);
		CYahelBrush(COLORREF solidColor,bool sysColor);
		~CYahelBrush();

		inline operator HBRUSH() const{ return handle; }
		inline operator COLORREF() const{ return lbColor; }
		CYahelBrush &operator=(CYahelBrush &&r);
	};

	class CYahelFont{
		const HFONT handle;
		int charAvgWidth,charHeight;
	public:
		struct TLogFont:public LOGFONTW{
			TLogFont(LPCWSTR face,int pointHeight);
			TLogFont(HFONT hFont);
		};

		CYahelFont(const LOGFONTW &lf);
		~CYahelFont();

		inline operator HFONT() const{ return handle; }

		inline int GetCharAvgWidth() const{ return charAvgWidth; }
		inline int GetCharHeight() const{ return charHeight; }
		SIZE GetTextSize(LPCTSTR text,int textLength) const;
		SIZE GetTextSize(LPCTSTR text) const;
	};

	class CYahelContextMenu{
		const HMENU hResourceMenu;
		const HMENU handle;
	public:
		CYahelContextMenu(UINT idMenuRes);
		~CYahelContextMenu();

		inline operator HMENU() const{ return handle; }

		void UpdateUi(const IOwner &owner) const;
	};

	class CYahelDialog{
		static INT_PTR WINAPI DialogProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
	protected:
		HWND hDlg;

		HWND GetDlgItemHwnd(WORD id) const{ return ::GetDlgItem(hDlg,id); }
		bool EnableDlgItem(WORD id,bool enabled=true) const;
		bool EnableDlgItems(PCWORD pIds,bool enabled) const;
		inline bool IsDlgButtonChecked(WORD id) const{ return ::IsDlgButtonChecked(hDlg,id)!=BST_UNCHECKED; }
		inline void CheckDlgButton(WORD id,bool checked) const{ ::CheckDlgButton(hDlg,id,checked*BST_CHECKED); }
		void InitDlgItemText(WORD id,...) const;
		int GetDlgItemTextLength(WORD id) const;
		inline int SetDlgItemText(WORD id,LPCTSTR text) const{ return ::SetDlgItemText( hDlg, id, text ); }
		inline int SetDlgItemInt(WORD id,int i) const{ return Gui::SetDlgItemInt( hDlg, id, i, Gui::Signed ); }
		inline int SetDlgItemInt(WORD id,int i,bool hexa) const{ return Gui::SetDlgItemInt( hDlg, id, i, hexa ); }
		HWND FocusDlgItem(WORD id) const;
		inline LRESULT SendCommand(WPARAM wParam,LPARAM lParam=0) const{ return ::SendMessage( hDlg, WM_COMMAND, wParam, lParam ); }
		RECT MapDlgItemClientRect(WORD id) const;
		virtual bool InitDialog()=0;
		virtual bool ValidateDialog()=0;
		virtual bool OnCommand(WPARAM wParam,LPARAM lParam)=0;

		CYahelDialog();

		template <size_t N>
		int GetDlgItemText(WORD id,TCHAR (&buffer)[N]) const{
			return ::GetDlgItemText( hDlg, id, buffer, N );
		}
	public:
		UINT_PTR DoModal(UINT nIDTemplate,HWND hParent=0);
	};

	class CViewportOrg{
		const HDC dc;
		POINT pt0;
	public:
		CViewportOrg(HDC dc,TRow r,TCol c,const CYahelFont &font);
		~CViewportOrg();
	};



	POINT MakePoint(LPARAM lParam);
	COLORREF GetSaturatedColor(COLORREF color,float saturationFactor);
	COLORREF GetBlendedColor(COLORREF color1,COLORREF color2,float blendFactor=.5f);
	void RandomizeData(PVOID buffer,TPosition nBytes);

}
}

#endif // UTILS_H
