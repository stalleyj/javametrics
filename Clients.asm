; Listing generated by Microsoft (R) Optimizing Compiler Version 16.00.40219.01 

	TITLE	C:\workspaces\newagent2\Paho\org.eclipse.paho.mqtt.c\src\Clients.c
	.686P
	.XMM
	include listing.inc
	.model	flat

INCLUDELIB MSVCRT
INCLUDELIB OLDNAMES

PUBLIC	_clientIDCompare
EXTRN	_strcmp:PROC
; Function compile flags: /Odtp
_TEXT	SEGMENT
_client$ = -4						; size = 4
_a$ = 8							; size = 4
_b$ = 12						; size = 4
_clientIDCompare PROC
; File c:\workspaces\newagent2\paho\org.eclipse.paho.mqtt.c\src\clients.c
; Line 37
	push	ebp
	mov	ebp, esp
	push	ecx
; Line 38
	mov	eax, DWORD PTR _a$[ebp]
	mov	DWORD PTR _client$[ebp], eax
; Line 40
	mov	ecx, DWORD PTR _b$[ebp]
	push	ecx
	mov	edx, DWORD PTR _client$[ebp]
	mov	eax, DWORD PTR [edx]
	push	eax
	call	_strcmp
	add	esp, 8
	neg	eax
	sbb	eax, eax
	add	eax, 1
; Line 41
	mov	esp, ebp
	pop	ebp
	ret	0
_clientIDCompare ENDP
_TEXT	ENDS
PUBLIC	_clientSocketCompare
; Function compile flags: /Odtp
_TEXT	SEGMENT
_client$ = -4						; size = 4
_a$ = 8							; size = 4
_b$ = 12						; size = 4
_clientSocketCompare PROC
; Line 51
	push	ebp
	mov	ebp, esp
	push	ecx
; Line 52
	mov	eax, DWORD PTR _a$[ebp]
	mov	DWORD PTR _client$[ebp], eax
; Line 54
	mov	ecx, DWORD PTR _client$[ebp]
	mov	edx, DWORD PTR _b$[ebp]
	mov	eax, DWORD PTR [ecx+16]
	xor	ecx, ecx
	cmp	eax, DWORD PTR [edx]
	sete	cl
	mov	eax, ecx
; Line 55
	mov	esp, ebp
	pop	ebp
	ret	0
_clientSocketCompare ENDP
_TEXT	ENDS
END
