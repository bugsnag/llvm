; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-pc-linux-gnu | FileCheck %s
; FIXME: This should not load or store the top part of *P.

define void @test(i64* %P) nounwind  {
; CHECK-LABEL: test:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl (%eax), %ecx
; CHECK-NEXT:    movl 4(%eax), %edx
; CHECK-NEXT:    xorl $1, %ecx
; CHECK-NEXT:    orl $2, %ecx
; CHECK-NEXT:    movl %edx, 4(%eax)
; CHECK-NEXT:    movl %ecx, (%eax)
; CHECK-NEXT:    retl
        %tmp1 = load i64, i64* %P, align 8
        %tmp2 = xor i64 %tmp1, 1
        %tmp3 = or i64 %tmp2, 2
        store i64 %tmp3, i64* %P, align 8
        ret void
}

