define i32 @main() {
    one:
        %a = add i32 0, 1
        br label %end

    end:
        ret i32 %a
}
