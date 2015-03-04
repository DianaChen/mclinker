; RUN: %LLC -mtriple="arm-none-linux-gnueabi" -march=arm \
; RUN: -filetype=obj -relocation-model=pic %s -o %t.1.o
; RUN: %LLC -mtriple="arm-none-linux-gnueabi" -march=arm \
; RUN: -filetype=obj -relocation-model=pic %s -o %t.2.o
; RUN: %MCLinker -mtriple="arm-none-linux-gnueabi" -march=arm \
; RUN: %t.1.o %t.2.o -z muldefs -o %t.so -shared

define i32 @_Z1fv() nounwind uwtable ssp {
  %b = alloca i32, align 4
  store i32 100, i32* %b, align 4
  %1 = load i32, i32* %b, align 4
  ret i32 %1
}
