module __if_add_proc__if_add_proc_wrapper_0_next__1(
  input wire clk,
  input wire reset
);
  reg p0_valid;
  reg p1_valid;
  wire p1_enable;
  wire p0_stage_done;
  wire p0_enable;
  assign p1_enable = 1'h1;
  assign p0_stage_done = 1'h1;
  assign p0_enable = 1'h1;
  always @ (posedge clk) begin
    if (reset) begin
      p0_valid <= 1'h0;
      p1_valid <= 1'h0;
    end else begin
      p0_valid <= p0_enable ? p0_stage_done : p0_valid;
      p1_valid <= p1_enable ? p0_valid : p1_valid;
    end
  end
endmodule


module __if_add_proc__if_add_proc_wrapper__if_add_proc_0__3_next(
  input wire clk,
  input wire reset,
  input wire [31:0] if_add_proc__input_b,
  input wire if_add_proc__input_b_vld,
  input wire [31:0] if_add_proc__input_a,
  input wire if_add_proc__input_a_vld,
  input wire if_add_proc__output_rdy,
  output wire [31:0] if_add_proc__output,
  output wire if_add_proc__output_vld,
  output wire if_add_proc__input_b_rdy,
  output wire if_add_proc__input_a_rdy
);
  function automatic [1:0] priority_sel_2b_2way (input reg [1:0] sel, input reg [1:0] case0, input reg [1:0] case1, input reg [1:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_2b_2way = case0;
        end
        2'b10: begin
          priority_sel_2b_2way = case1;
        end
        2'b00: begin
          priority_sel_2b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_2b_2way = 2'dx;
        end
      endcase
    end
  endfunction
  function automatic priority_sel_1b_4way (input reg [3:0] sel, input reg case0, input reg case1, input reg case2, input reg case3, input reg default_value);
    begin
      casez (sel)
        4'b???1: begin
          priority_sel_1b_4way = case0;
        end
        4'b??10: begin
          priority_sel_1b_4way = case1;
        end
        4'b?100: begin
          priority_sel_1b_4way = case2;
        end
        4'b1000: begin
          priority_sel_1b_4way = case3;
        end
        4'b0000: begin
          priority_sel_1b_4way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_1b_4way = 1'dx;
        end
      endcase
    end
  endfunction
  function automatic priority_sel_1b_3way (input reg [2:0] sel, input reg case0, input reg case1, input reg case2, input reg default_value);
    begin
      casez (sel)
        3'b??1: begin
          priority_sel_1b_3way = case0;
        end
        3'b?10: begin
          priority_sel_1b_3way = case1;
        end
        3'b100: begin
          priority_sel_1b_3way = case2;
        end
        3'b000: begin
          priority_sel_1b_3way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_1b_3way = 1'dx;
        end
      endcase
    end
  endfunction
  function automatic [2:0] priority_sel_3b_2way (input reg [1:0] sel, input reg [2:0] case0, input reg [2:0] case1, input reg [2:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_3b_2way = case0;
        end
        2'b10: begin
          priority_sel_3b_2way = case1;
        end
        2'b00: begin
          priority_sel_3b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_3b_2way = 3'dx;
        end
      endcase
    end
  endfunction
  function automatic [1:0] priority_sel_2b_4way (input reg [3:0] sel, input reg [1:0] case0, input reg [1:0] case1, input reg [1:0] case2, input reg [1:0] case3, input reg [1:0] default_value);
    begin
      casez (sel)
        4'b???1: begin
          priority_sel_2b_4way = case0;
        end
        4'b??10: begin
          priority_sel_2b_4way = case1;
        end
        4'b?100: begin
          priority_sel_2b_4way = case2;
        end
        4'b1000: begin
          priority_sel_2b_4way = case3;
        end
        4'b0000: begin
          priority_sel_2b_4way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_2b_4way = 2'dx;
        end
      endcase
    end
  endfunction
  function automatic [3:0] priority_sel_4b_2way (input reg [1:0] sel, input reg [3:0] case0, input reg [3:0] case1, input reg [3:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_4b_2way = case0;
        end
        2'b10: begin
          priority_sel_4b_2way = case1;
        end
        2'b00: begin
          priority_sel_4b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_4b_2way = 4'dx;
        end
      endcase
    end
  endfunction
  function automatic priority_sel_1b_2way (input reg [1:0] sel, input reg case0, input reg case1, input reg default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_1b_2way = case0;
        end
        2'b10: begin
          priority_sel_1b_2way = case1;
        end
        2'b00: begin
          priority_sel_1b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_1b_2way = 1'dx;
        end
      endcase
    end
  endfunction
  wire [31:0] __if_add_proc__input_b_reg_init = {1'h0, 8'h00, 23'h00_0000};
  wire [31:0] __if_add_proc__input_a_reg_init = {1'h0, 8'h00, 23'h00_0000};
  wire [31:0] __if_add_proc__output_reg_init = {1'h0, 8'h00, 23'h00_0000};
  reg p0_is_result_nan;
  reg [7:0] p0_result_exponent__2;
  reg p0_or_24869;
  reg [22:0] p0_result_fraction;
  reg p0_result_sign__2;
  reg [7:0] ____state_1;
  reg [22:0] ____state_2;
  reg ____state_0;
  reg [1:0] ____state_3;
  reg [22:0] p1_____state_2__1;
  reg p1_is_result_nan__1;
  reg p1_eq_25241;
  reg [22:0] p1_result_fraction__8;
  reg p1_or_25249;
  reg p1_not_25248;
  reg p1_sel_25259;
  reg [7:0] p1_sel_25263;
  reg [2:0] p1_add_25264;
  reg p0_valid;
  reg p1_valid;
  reg [31:0] __if_add_proc__input_b_reg;
  reg __if_add_proc__input_b_valid_reg;
  reg [31:0] __if_add_proc__input_a_reg;
  reg __if_add_proc__input_a_valid_reg;
  reg [31:0] __if_add_proc__output_reg;
  reg __if_add_proc__output_valid_reg;
  wire [8:0] sum__1;
  wire [7:0] b_bexp__4;
  wire [22:0] result_fraction__3;
  wire [22:0] FRACTION_HIGH_BIT;
  wire [7:0] a_bexp__2;
  wire [22:0] result_fraction__4;
  wire [7:0] acc_bexp;
  wire [7:0] F32_ZERO__1_bexp__6;
  wire [22:0] acc_fraction;
  wire [7:0] diff_bexp;
  wire [7:0] F32_ZERO__1_bexp__5;
  wire [8:0] sum;
  wire [23:0] fraction_x__2;
  wire [22:0] diff_fraction;
  wire [23:0] fraction_x__3;
  wire [2:0] addend_x__2_squeezed_const_lsb_bits__3;
  wire [23:0] fraction_y__2;
  wire [23:0] sign_ext_24933;
  wire [7:0] incremented_sum__3;
  wire [22:0] b_fraction__4;
  wire [22:0] tuple_index_24553;
  wire [7:0] a_bexp__4;
  wire [7:0] F32_ZERO__1_bexp__3;
  wire [27:0] wide_x__1;
  wire [23:0] fraction_y__3;
  wire [2:0] addend_x__2_squeezed_const_lsb_bits__2;
  wire [7:0] acc_bexpbs_difference__1;
  wire [22:0] a_fraction__1;
  wire [7:0] b_bexp__5;
  wire [7:0] F32_ZERO__1_bexp__2;
  wire acc_sign;
  wire diff_sign;
  wire [27:0] neg_24945;
  wire [27:0] wide_y__1;
  wire [23:0] fraction_x;
  wire [22:0] b_fraction__5;
  wire [24:0] wide_x__1_squeezed;
  wire [27:0] shrl_24952;
  wire [27:0] add_24953;
  wire b_sign__2;
  wire [23:0] fraction_x__1;
  wire [2:0] addend_x__2_squeezed_const_lsb_bits__1;
  wire [23:0] fraction_y;
  wire [23:0] sign_ext_24573;
  wire [7:0] incremented_sum__2;
  wire [24:0] accddend_x__1_squeezed;
  wire tuple_index_24577;
  wire [27:0] wide_x;
  wire [23:0] fraction_y__1;
  wire [2:0] addend_x__2_squeezed_const_lsb_bits;
  wire [7:0] a_bexpbs_difference__2;
  wire a_sign__1;
  wire b_sign__3;
  wire [27:0] neg_24586;
  wire [27:0] wide_y;
  wire [25:0] add_24962;
  wire sticky__1;
  wire [24:0] wide_x_squeezed;
  wire [27:0] shrl_24593;
  wire [27:0] add_24594;
  wire [24:0] addend_x__2_squeezed;
  wire [27:0] concat_24968;
  wire [25:0] add_24603;
  wire sticky;
  wire [27:0] accbs_fraction;
  wire carry_bit__1;
  wire [27:0] concat_24609;
  wire nor_25000;
  wire nor_25001;
  wire nor_25003;
  wire nor_25004;
  wire nor_25009;
  wire nor_25010;
  wire nor_25015;
  wire nor_25018;
  wire nor_25019;
  wire nor_25020;
  wire nor_25026;
  wire nor_25027;
  wire and_25028;
  wire and_25030;
  wire nor_25035;
  wire and_25036;
  wire nor_25039;
  wire and_25043;
  wire nor_25044;
  wire and_25048;
  wire nor_25050;
  wire [27:0] abs_fraction__1;
  wire and_25057;
  wire [1:0] unexpand_for___state_3_next_case_1__2;
  wire and_25075;
  wire carry_bit;
  wire and_25093;
  wire and_25095;
  wire nor_25101;
  wire and_25102;
  wire [1:0] priority_sel_25109;
  wire nor_24641;
  wire nor_24642;
  wire nor_24644;
  wire nor_24645;
  wire nor_24650;
  wire nor_24651;
  wire nor_24656;
  wire nor_24659;
  wire nor_24660;
  wire nor_24661;
  wire nor_24667;
  wire [1:0] unexpand_for___state_3_next_case_1__5;
  wire [1:0] unexpand_for___state_3_next_case_1__6;
  wire nor_24668;
  wire and_24669;
  wire and_24671;
  wire nor_24676;
  wire and_24677;
  wire nor_24680;
  wire and_24684;
  wire nor_24685;
  wire and_24689;
  wire nor_24691;
  wire and_25129;
  wire and_24698;
  wire [1:0] unexpand_for___state_3_next_case_1__1;
  wire and_24716;
  wire and_24734;
  wire and_24736;
  wire nor_24742;
  wire and_24743;
  wire [1:0] priority_sel_24750;
  wire [3:0] leading_zeroes__1__0_to_4;
  wire [1:0] unexpand_for___state_3_next_case_1__3;
  wire [1:0] unexpand_for___state_3_next_case_1__4;
  wire [4:0] leading_zeroes__1;
  wire and_24770;
  wire [28:0] cancel_fraction__2;
  wire [26:0] cancel_fraction__3;
  wire [26:0] carry_fraction__3;
  wire [3:0] leading_zeroes__0_to_4;
  wire [26:0] shifted_fraction__1;
  wire [4:0] leading_zeroes;
  wire [2:0] normal_chunk__1;
  wire [2:0] fraction_shift__5;
  wire [1:0] half_way_chunk__1;
  wire [28:0] cancel_fraction;
  wire [24:0] concat_25156;
  wire [26:0] cancel_fraction__1;
  wire [26:0] carry_fraction__1;
  wire do_round_up__1;
  wire [24:0] add_25159;
  wire [26:0] shifted_fraction;
  wire [24:0] rounded_fraction__1_squeezed_portion_3_width_25;
  wire [2:0] normal_chunk;
  wire [2:0] fraction_shift__3;
  wire [1:0] half_way_chunk;
  wire [7:0] F32_ZERO__1_bexp__8;
  wire rounding_carry__1;
  wire [24:0] concat_24797;
  wire do_round_up;
  wire [24:0] add_24800;
  wire [8:0] add_25170;
  wire [24:0] rounded_fraction_squeezed_portion_3_width_25;
  wire fraction_is_zero__1;
  wire [7:0] F32_ZERO__1_bexp__7;
  wire rounding_carry;
  wire [9:0] add_25178;
  wire [9:0] wide_exponent__3;
  wire [8:0] add_24811;
  wire [9:0] wide_exponent__4;
  wire fraction_is_zero;
  wire [7:0] MAX_EXPONENT__4;
  wire [7:0] MAX_EXPONENT__5;
  wire [9:0] add_24819;
  wire eq_25193;
  wire eq_25194;
  wire eq_25195;
  wire eq_25196;
  wire if_add_proc__output_valid_inv;
  wire [9:0] wide_exponent;
  wire [7:0] MAX_EXPONENT__3;
  wire __if_add_proc__output_vld_buf;
  wire if_add_proc__output_valid_load_en;
  wire [7:0] MAX_EXPONENT;
  wire [7:0] MAX_EXPONENT__1;
  wire [9:0] wide_exponent__1;
  wire gt_fraction;
  wire [8:0] wide_exponent__5;
  wire if_add_proc__output_load_en;
  wire eq_24829;
  wire eq_24830;
  wire eq_24831;
  wire eq_24832;
  wire has_pos_inf__1;
  wire has_neg_inf__1;
  wire [7:0] F32_ZERO__1_bexp__4;
  wire eq_25241;
  wire eq_exp;
  wire p2_stage_done;
  wire p2_not_valid;
  wire is_result_nan__1;
  wire is_operand_inf__1;
  wire result_sign__3;
  wire and_reduce_25217;
  wire not_25248;
  wire or_25249;
  wire p1_enable;
  wire [2:0] fraction_shift__8;
  wire [2:0] fraction_shift__7;
  wire has_pos_inf;
  wire has_neg_inf;
  wire [8:0] wide_exponent__2;
  wire result_sign__4;
  wire or_25250;
  wire and_25255;
  wire or_25225;
  wire p1_data_enable;
  wire p1_not_valid;
  wire [27:0] rounded_fraction__1;
  wire [2:0] fraction_shift__6;
  wire [2:0] fraction_shift__2;
  wire [2:0] fraction_shift__4;
  wire result_sign__5;
  wire and_25262;
  wire p0_enable;
  wire p0_all_active_inputs_valid;
  wire [22:0] FRACTION_HIGH_BIT__1;
  wire [27:0] shrl_25236;
  wire [7:0] MAX_EXPONENT__6;
  wire is_result_nan;
  wire is_operand_inf;
  wire and_reduce_24858;
  wire [27:0] rounded_fraction;
  wire [2:0] fraction_shift__1;
  wire result_sign;
  wire sel_25259;
  wire and_25368;
  wire and_25468;
  wire and_25469;
  wire and_25377;
  wire and_25470;
  wire and_25471;
  wire and_25472;
  wire and_25387;
  wire and_25388;
  wire [2:0] add_25264;
  wire p0_data_enable;
  wire if_add_proc__input_b_valid_inv;
  wire if_add_proc__input_a_valid_inv;
  wire [22:0] result_fraction__9;
  wire [22:0] result_fraction__5;
  wire [7:0] result_exponent__1;
  wire [7:0] MAX_EXPONENT__2;
  wire [27:0] shrl_24870;
  wire result_sign__1;
  wire [2:0] concat_25373;
  wire [7:0] MAX_EXPONENT__7;
  wire [7:0] F32_ZERO__1_bexp__1;
  wire [3:0] concat_25383;
  wire [22:0] FRACTION_HIGH_BIT__2;
  wire [1:0] concat_25389;
  wire [1:0] unexpand_for___state_3_next_case_1;
  wire [1:0] unexpand_for___state_3_next_case_0;
  wire if_add_proc__input_b_valid_load_en;
  wire if_add_proc__input_a_valid_load_en;
  wire [22:0] sel_25310;
  wire [31:0] count__1;
  wire [22:0] result_fraction__8;
  wire [7:0] sel_25263;
  wire [7:0] result_exponent__2;
  wire or_24869;
  wire [22:0] result_fraction;
  wire result_sign__2;
  wire nor_25272;
  wire [7:0] one_hot_sel_25374;
  wire or_25375;
  wire [22:0] one_hot_sel_25384;
  wire or_25385;
  wire [1:0] one_hot_sel_25390;
  wire or_25391;
  wire if_add_proc__input_b_load_en;
  wire if_add_proc__input_a_load_en;
  wire [31:0] acc__1;
  assign sum__1 = {1'h0, ____state_1} + {1'h0, ~p0_result_exponent__2};
  assign b_bexp__4 = __if_add_proc__input_b_reg[30:23];
  assign result_fraction__3 = p0_result_fraction & {23{~p0_or_24869}};
  assign FRACTION_HIGH_BIT = 23'h40_0000;
  assign a_bexp__2 = __if_add_proc__input_a_reg[30:23];
  assign result_fraction__4 = p0_is_result_nan ? FRACTION_HIGH_BIT : result_fraction__3;
  assign acc_bexp = sum__1[8] ? ____state_1 : p0_result_exponent__2;
  assign F32_ZERO__1_bexp__6 = 8'h00;
  assign acc_fraction = sum__1[8] ? ____state_2 : result_fraction__4;
  assign diff_bexp = sum__1[8] ? p0_result_exponent__2 : ____state_1;
  assign F32_ZERO__1_bexp__5 = 8'h00;
  assign sum = {1'h0, a_bexp__2} + {1'h0, ~b_bexp__4};
  assign fraction_x__2 = {1'h1, acc_fraction};
  assign diff_fraction = sum__1[8] ? result_fraction__4 : ____state_2;
  assign fraction_x__3 = fraction_x__2 & {24{acc_bexp != F32_ZERO__1_bexp__6}};
  assign addend_x__2_squeezed_const_lsb_bits__3 = 3'h0;
  assign fraction_y__2 = {1'h1, diff_fraction};
  assign sign_ext_24933 = {24{diff_bexp != F32_ZERO__1_bexp__5}};
  assign incremented_sum__3 = sum__1[7:0] + 8'h01;
  assign b_fraction__4 = __if_add_proc__input_b_reg[22:0];
  assign tuple_index_24553 = __if_add_proc__input_a_reg[22:0];
  assign a_bexp__4 = sum[8] ? a_bexp__2 : b_bexp__4;
  assign F32_ZERO__1_bexp__3 = 8'h00;
  assign wide_x__1 = {1'h0, fraction_x__3, addend_x__2_squeezed_const_lsb_bits__3};
  assign fraction_y__3 = fraction_y__2 & sign_ext_24933;
  assign addend_x__2_squeezed_const_lsb_bits__2 = 3'h0;
  assign acc_bexpbs_difference__1 = sum__1[8] ? incremented_sum__3 : ~sum__1[7:0];
  assign a_fraction__1 = sum[8] ? tuple_index_24553 : b_fraction__4;
  assign b_bexp__5 = sum[8] ? b_bexp__4 : a_bexp__2;
  assign F32_ZERO__1_bexp__2 = 8'h00;
  assign acc_sign = sum__1[8] ? ____state_0 : p0_result_sign__2;
  assign diff_sign = sum__1[8] ? p0_result_sign__2 : ____state_0;
  assign neg_24945 = -wide_x__1;
  assign wide_y__1 = {1'h0, fraction_y__3, addend_x__2_squeezed_const_lsb_bits__2};
  assign fraction_x = {1'h1, a_fraction__1};
  assign b_fraction__5 = sum[8] ? b_fraction__4 : tuple_index_24553;
  assign wide_x__1_squeezed = {1'h0, fraction_x__3};
  assign shrl_24952 = acc_bexpbs_difference__1 >= 8'h1c ? 28'h000_0000 : wide_y__1 >> acc_bexpbs_difference__1;
  assign add_24953 = (acc_bexpbs_difference__1 >= 8'h1c ? 28'h000_0000 : 28'h000_0001 << acc_bexpbs_difference__1) + 28'hfff_ffff;
  assign b_sign__2 = __if_add_proc__input_b_reg[31:31];
  assign fraction_x__1 = fraction_x & {24{a_bexp__4 != F32_ZERO__1_bexp__3}};
  assign addend_x__2_squeezed_const_lsb_bits__1 = 3'h0;
  assign fraction_y = {1'h1, b_fraction__5};
  assign sign_ext_24573 = {24{b_bexp__5 != F32_ZERO__1_bexp__2}};
  assign incremented_sum__2 = sum[7:0] + 8'h01;
  assign accddend_x__1_squeezed = acc_sign ^ diff_sign ? neg_24945[27:3] : wide_x__1_squeezed;
  assign tuple_index_24577 = __if_add_proc__input_a_reg[31:31];
  assign wide_x = {1'h0, fraction_x__1, addend_x__2_squeezed_const_lsb_bits__1};
  assign fraction_y__1 = fraction_y & sign_ext_24573;
  assign addend_x__2_squeezed_const_lsb_bits = 3'h0;
  assign a_bexpbs_difference__2 = sum[8] ? incremented_sum__2 : ~sum[7:0];
  assign a_sign__1 = sum[8] ? tuple_index_24577 : ~b_sign__2;
  assign b_sign__3 = sum[8] ? ~b_sign__2 : tuple_index_24577;
  assign neg_24586 = -wide_x;
  assign wide_y = {1'h0, fraction_y__1, addend_x__2_squeezed_const_lsb_bits};
  assign add_24962 = {{1{accddend_x__1_squeezed[24]}}, accddend_x__1_squeezed} + {1'h0, shrl_24952[27:3]};
  assign sticky__1 = (fraction_y__2 & sign_ext_24933 & add_24953[26:3]) != 24'h00_0000;
  assign wide_x_squeezed = {1'h0, fraction_x__1};
  assign shrl_24593 = a_bexpbs_difference__2 >= 8'h1c ? 28'h000_0000 : wide_y >> a_bexpbs_difference__2;
  assign add_24594 = (a_bexpbs_difference__2 >= 8'h1c ? 28'h000_0000 : 28'h000_0001 << a_bexpbs_difference__2) + 28'hfff_ffff;
  assign addend_x__2_squeezed = a_sign__1 ^ b_sign__3 ? neg_24586[27:3] : wide_x_squeezed;
  assign concat_24968 = {add_24962[24:0], shrl_24952[2:1], shrl_24952[0] | sticky__1};
  assign add_24603 = {{1{addend_x__2_squeezed[24]}}, addend_x__2_squeezed} + {1'h0, shrl_24593[27:3]};
  assign sticky = (fraction_y & sign_ext_24573 & add_24594[26:3]) != 24'h00_0000;
  assign accbs_fraction = add_24962[25] ? -concat_24968 : concat_24968;
  assign carry_bit__1 = accbs_fraction[27];
  assign concat_24609 = {add_24603[24:0], shrl_24593[2:1], shrl_24593[0] | sticky};
  assign nor_25000 = ~(accbs_fraction[11] | accbs_fraction[10]);
  assign nor_25001 = ~(accbs_fraction[9] | accbs_fraction[8]);
  assign nor_25003 = ~(accbs_fraction[1] | accbs_fraction[0]);
  assign nor_25004 = ~(accbs_fraction[3] | accbs_fraction[2]);
  assign nor_25009 = ~(accbs_fraction[5] | accbs_fraction[4]);
  assign nor_25010 = ~(accbs_fraction[7] | accbs_fraction[6]);
  assign nor_25015 = ~(accbs_fraction[17] | accbs_fraction[16]);
  assign nor_25018 = ~(accbs_fraction[13] | accbs_fraction[12]);
  assign nor_25019 = ~(carry_bit__1 | accbs_fraction[26]);
  assign nor_25020 = ~(accbs_fraction[25] | accbs_fraction[24]);
  assign nor_25026 = ~(accbs_fraction[21] | accbs_fraction[20]);
  assign nor_25027 = ~(accbs_fraction[23] | accbs_fraction[22]);
  assign and_25028 = nor_25000 & nor_25001;
  assign and_25030 = nor_25004 & nor_25003;
  assign nor_25035 = ~(accbs_fraction[7] | accbs_fraction[6] | nor_25009);
  assign and_25036 = nor_25010 & nor_25009;
  assign nor_25039 = ~(accbs_fraction[11] | ~accbs_fraction[10]);
  assign and_25043 = ~(accbs_fraction[19] | accbs_fraction[18]) & nor_25015;
  assign nor_25044 = ~(accbs_fraction[15] | accbs_fraction[14]);
  assign and_25048 = nor_25019 & nor_25020;
  assign nor_25050 = ~(carry_bit__1 | ~accbs_fraction[26]);
  assign abs_fraction__1 = add_24603[25] ? -concat_24609 : concat_24609;
  assign and_25057 = nor_25027 & nor_25026;
  assign unexpand_for___state_3_next_case_1__2 = 2'h0;
  assign and_25075 = nor_25044 & nor_25018;
  assign carry_bit = abs_fraction__1[27];
  assign and_25093 = and_25048 & and_25057;
  assign and_25095 = and_25028 & and_25036;
  assign nor_25101 = ~(~and_25043 | and_25075);
  assign and_25102 = and_25043 & and_25075;
  assign priority_sel_25109 = priority_sel_2b_2way({~(carry_bit__1 | accbs_fraction[26] | nor_25020), and_25048}, {nor_25050, 1'h0}, {1'h1, ~(accbs_fraction[25] | ~accbs_fraction[24])}, {nor_25019, nor_25050});
  assign nor_24641 = ~(abs_fraction__1[11] | abs_fraction__1[10]);
  assign nor_24642 = ~(abs_fraction__1[9] | abs_fraction__1[8]);
  assign nor_24644 = ~(abs_fraction__1[1] | abs_fraction__1[0]);
  assign nor_24645 = ~(abs_fraction__1[3] | abs_fraction__1[2]);
  assign nor_24650 = ~(abs_fraction__1[5] | abs_fraction__1[4]);
  assign nor_24651 = ~(abs_fraction__1[7] | abs_fraction__1[6]);
  assign nor_24656 = ~(abs_fraction__1[17] | abs_fraction__1[16]);
  assign nor_24659 = ~(abs_fraction__1[13] | abs_fraction__1[12]);
  assign nor_24660 = ~(carry_bit | abs_fraction__1[26]);
  assign nor_24661 = ~(abs_fraction__1[25] | abs_fraction__1[24]);
  assign nor_24667 = ~(abs_fraction__1[21] | abs_fraction__1[20]);
  assign unexpand_for___state_3_next_case_1__5 = 2'h0;
  assign unexpand_for___state_3_next_case_1__6 = 2'h0;
  assign nor_24668 = ~(abs_fraction__1[23] | abs_fraction__1[22]);
  assign and_24669 = nor_24641 & nor_24642;
  assign and_24671 = nor_24645 & nor_24644;
  assign nor_24676 = ~(abs_fraction__1[7] | abs_fraction__1[6] | nor_24650);
  assign and_24677 = nor_24651 & nor_24650;
  assign nor_24680 = ~(abs_fraction__1[11] | ~abs_fraction__1[10]);
  assign and_24684 = ~(abs_fraction__1[19] | abs_fraction__1[18]) & nor_24656;
  assign nor_24685 = ~(abs_fraction__1[15] | abs_fraction__1[14]);
  assign and_24689 = nor_24660 & nor_24661;
  assign nor_24691 = ~(carry_bit | ~abs_fraction__1[26]);
  assign and_25129 = and_25093 & and_25102;
  assign and_24698 = nor_24668 & nor_24667;
  assign unexpand_for___state_3_next_case_1__1 = 2'h0;
  assign and_24716 = nor_24685 & nor_24659;
  assign and_24734 = and_24689 & and_24698;
  assign and_24736 = and_24669 & and_24677;
  assign nor_24742 = ~(~and_24684 | and_24716);
  assign and_24743 = and_24684 & and_24716;
  assign priority_sel_24750 = priority_sel_2b_2way({~(carry_bit | abs_fraction__1[26] | nor_24661), and_24689}, {nor_24691, 1'h0}, {1'h1, ~(abs_fraction__1[25] | ~abs_fraction__1[24])}, {nor_24660, nor_24691});
  assign leading_zeroes__1__0_to_4 = priority_sel_4b_2way({~(~and_25093 | and_25102), and_25129}, {and_25095, priority_sel_3b_2way({~(~and_25028 | and_25036), and_25095}, {and_25030, priority_sel_2b_2way({~(accbs_fraction[3] | accbs_fraction[2] | nor_25003), and_25030}, unexpand_for___state_3_next_case_1__2, {1'h1, ~(accbs_fraction[1] | ~accbs_fraction[0])}, {nor_25004, ~(accbs_fraction[3] | ~accbs_fraction[2])})}, {1'h1, nor_25035, priority_sel_1b_4way({~(accbs_fraction[7] | ~accbs_fraction[6]), nor_25010, nor_25035, and_25036}, 1'h0, ~(accbs_fraction[5] | ~accbs_fraction[4]), 1'h0, 1'h1, 1'h0)}, {and_25028, priority_sel_2b_2way({~(accbs_fraction[11] | accbs_fraction[10] | nor_25001), and_25028}, {nor_25039, 1'h0}, {1'h1, ~(accbs_fraction[9] | ~accbs_fraction[8])}, {nor_25000, nor_25039})})}, {1'h1, nor_25101, priority_sel_2b_4way({~(accbs_fraction[19] | accbs_fraction[18] | nor_25015), and_25043, nor_25101, and_25102}, unexpand_for___state_3_next_case_1__5, {nor_25044, priority_sel_1b_3way({~(accbs_fraction[15] | ~accbs_fraction[14]), nor_25044, ~(accbs_fraction[15] | accbs_fraction[14] | nor_25018)}, ~(accbs_fraction[13] | ~accbs_fraction[12]), 1'h0, 1'h1, 1'h0)}, unexpand_for___state_3_next_case_1__6, {1'h1, ~(accbs_fraction[17] | ~accbs_fraction[16])}, {1'h0, ~(accbs_fraction[19] | ~accbs_fraction[18])})}, {and_25093, priority_sel_3b_2way({~(~and_25048 | and_25057), and_25093}, {priority_sel_25109, 1'h0}, {1'h1, nor_25027, priority_sel_1b_3way({~(accbs_fraction[23] | ~accbs_fraction[22]), nor_25027, ~(accbs_fraction[23] | accbs_fraction[22] | nor_25026)}, ~(accbs_fraction[21] | ~accbs_fraction[20]), 1'h0, 1'h1, 1'h0)}, {and_25048, priority_sel_25109})});
  assign unexpand_for___state_3_next_case_1__3 = 2'h0;
  assign unexpand_for___state_3_next_case_1__4 = 2'h0;
  assign leading_zeroes__1 = {and_25129, leading_zeroes__1__0_to_4};
  assign and_24770 = and_24734 & and_24743;
  assign cancel_fraction__2 = leading_zeroes__1 >= 5'h1d ? 29'h0000_0000 : {1'h0, accbs_fraction} << leading_zeroes__1;
  assign cancel_fraction__3 = cancel_fraction__2[27:1];
  assign carry_fraction__3 = {accbs_fraction[27:2], accbs_fraction[1] | accbs_fraction[0]};
  assign leading_zeroes__0_to_4 = priority_sel_4b_2way({~(~and_24734 | and_24743), and_24770}, {and_24736, priority_sel_3b_2way({~(~and_24669 | and_24677), and_24736}, {and_24671, priority_sel_2b_2way({~(abs_fraction__1[3] | abs_fraction__1[2] | nor_24644), and_24671}, unexpand_for___state_3_next_case_1__1, {1'h1, ~(abs_fraction__1[1] | ~abs_fraction__1[0])}, {nor_24645, ~(abs_fraction__1[3] | ~abs_fraction__1[2])})}, {1'h1, nor_24676, priority_sel_1b_4way({~(abs_fraction__1[7] | ~abs_fraction__1[6]), nor_24651, nor_24676, and_24677}, 1'h0, ~(abs_fraction__1[5] | ~abs_fraction__1[4]), 1'h0, 1'h1, 1'h0)}, {and_24669, priority_sel_2b_2way({~(abs_fraction__1[11] | abs_fraction__1[10] | nor_24642), and_24669}, {nor_24680, 1'h0}, {1'h1, ~(abs_fraction__1[9] | ~abs_fraction__1[8])}, {nor_24641, nor_24680})})}, {1'h1, nor_24742, priority_sel_2b_4way({~(abs_fraction__1[19] | abs_fraction__1[18] | nor_24656), and_24684, nor_24742, and_24743}, unexpand_for___state_3_next_case_1__3, {nor_24685, priority_sel_1b_3way({~(abs_fraction__1[15] | ~abs_fraction__1[14]), nor_24685, ~(abs_fraction__1[15] | abs_fraction__1[14] | nor_24659)}, ~(abs_fraction__1[13] | ~abs_fraction__1[12]), 1'h0, 1'h1, 1'h0)}, unexpand_for___state_3_next_case_1__4, {1'h1, ~(abs_fraction__1[17] | ~abs_fraction__1[16])}, {1'h0, ~(abs_fraction__1[19] | ~abs_fraction__1[18])})}, {and_24734, priority_sel_3b_2way({~(~and_24689 | and_24698), and_24734}, {priority_sel_24750, 1'h0}, {1'h1, nor_24668, priority_sel_1b_3way({~(abs_fraction__1[23] | ~abs_fraction__1[22]), nor_24668, ~(abs_fraction__1[23] | abs_fraction__1[22] | nor_24667)}, ~(abs_fraction__1[21] | ~abs_fraction__1[20]), 1'h0, 1'h1, 1'h0)}, {and_24689, priority_sel_24750})});
  assign shifted_fraction__1 = carry_bit__1 ? carry_fraction__3 : cancel_fraction__3;
  assign leading_zeroes = {and_24770, leading_zeroes__0_to_4};
  assign normal_chunk__1 = shifted_fraction__1[2:0];
  assign fraction_shift__5 = 3'h4;
  assign half_way_chunk__1 = shifted_fraction__1[3:2];
  assign cancel_fraction = leading_zeroes >= 5'h1d ? 29'h0000_0000 : {1'h0, abs_fraction__1} << leading_zeroes;
  assign concat_25156 = {1'h0, shifted_fraction__1[26:3]};
  assign cancel_fraction__1 = cancel_fraction[27:1];
  assign carry_fraction__1 = {abs_fraction__1[27:2], abs_fraction__1[1] | abs_fraction__1[0]};
  assign do_round_up__1 = normal_chunk__1 > fraction_shift__5 | half_way_chunk__1 == 2'h3;
  assign add_25159 = concat_25156 + 25'h000_0001;
  assign shifted_fraction = carry_bit ? carry_fraction__1 : cancel_fraction__1;
  assign rounded_fraction__1_squeezed_portion_3_width_25 = do_round_up__1 ? add_25159 : concat_25156;
  assign normal_chunk = shifted_fraction[2:0];
  assign fraction_shift__3 = 3'h4;
  assign half_way_chunk = shifted_fraction[3:2];
  assign F32_ZERO__1_bexp__8 = 8'h00;
  assign rounding_carry__1 = rounded_fraction__1_squeezed_portion_3_width_25[24];
  assign concat_24797 = {1'h0, shifted_fraction[26:3]};
  assign do_round_up = normal_chunk > fraction_shift__3 | half_way_chunk == 2'h3;
  assign add_24800 = concat_24797 + 25'h000_0001;
  assign add_25170 = {1'h0, acc_bexp} + {F32_ZERO__1_bexp__8, rounding_carry__1};
  assign rounded_fraction_squeezed_portion_3_width_25 = do_round_up ? add_24800 : concat_24797;
  assign fraction_is_zero__1 = add_24962 == 26'h000_0000 & ~(shrl_24952[1] | shrl_24952[2]) & ~(shrl_24952[0] | sticky__1);
  assign F32_ZERO__1_bexp__7 = 8'h00;
  assign rounding_carry = rounded_fraction_squeezed_portion_3_width_25[24];
  assign add_25178 = {1'h0, add_25170} + 10'h001;
  assign wide_exponent__3 = add_25178 - {5'h00, and_25129, leading_zeroes__1__0_to_4};
  assign add_24811 = {1'h0, a_bexp__4} + {F32_ZERO__1_bexp__7, rounding_carry};
  assign wide_exponent__4 = wide_exponent__3 & {10{~fraction_is_zero__1}};
  assign fraction_is_zero = add_24603 == 26'h000_0000 & ~(shrl_24593[1] | shrl_24593[2]) & ~(shrl_24593[0] | sticky);
  assign MAX_EXPONENT__4 = 8'hff;
  assign MAX_EXPONENT__5 = 8'hff;
  assign add_24819 = {1'h0, add_24811} + 10'h001;
  assign eq_25193 = acc_bexp == MAX_EXPONENT__4;
  assign eq_25194 = acc_fraction == 23'h00_0000;
  assign eq_25195 = diff_bexp == MAX_EXPONENT__5;
  assign eq_25196 = diff_fraction == 23'h00_0000;
  assign if_add_proc__output_valid_inv = ~__if_add_proc__output_valid_reg;
  assign wide_exponent = add_24819 - {5'h00, and_24770, leading_zeroes__0_to_4};
  assign MAX_EXPONENT__3 = 8'hff;
  assign __if_add_proc__output_vld_buf = p1_valid & p1_eq_25241;
  assign if_add_proc__output_valid_load_en = if_add_proc__output_rdy | if_add_proc__output_valid_inv;
  assign MAX_EXPONENT = 8'hff;
  assign MAX_EXPONENT__1 = 8'hff;
  assign wide_exponent__1 = wide_exponent & {10{~fraction_is_zero}};
  assign gt_fraction = result_fraction__4 != 23'h00_0000;
  assign wide_exponent__5 = wide_exponent__4[8:0] & {9{~wide_exponent__4[9]}};
  assign if_add_proc__output_load_en = __if_add_proc__output_vld_buf & if_add_proc__output_valid_load_en;
  assign eq_24829 = a_bexp__4 == MAX_EXPONENT;
  assign eq_24830 = a_fraction__1 == 23'h00_0000;
  assign eq_24831 = b_bexp__5 == MAX_EXPONENT__1;
  assign eq_24832 = b_fraction__5 == 23'h00_0000;
  assign has_pos_inf__1 = ~(~eq_25193 | ~eq_25194 | acc_sign) | ~(~eq_25195 | ~eq_25196 | diff_sign);
  assign has_neg_inf__1 = eq_25193 & eq_25194 & acc_sign | eq_25195 & eq_25196 & diff_sign;
  assign F32_ZERO__1_bexp__4 = 8'h00;
  assign eq_25241 = ____state_3 == 2'h2;
  assign eq_exp = p0_result_exponent__2 == F32_ZERO__1_bexp__4;
  assign p2_stage_done = p1_valid & (~p1_eq_25241 | if_add_proc__output_load_en);
  assign p2_not_valid = ~p1_valid;
  assign is_result_nan__1 = ~(~eq_25193 | eq_25194) | ~(~eq_25195 | eq_25196) | has_pos_inf__1 & has_neg_inf__1;
  assign is_operand_inf__1 = eq_25193 & eq_25194 | eq_25195 & eq_25196;
  assign result_sign__3 = priority_sel_1b_2way({add_24962[25], fraction_is_zero__1}, acc_sign & diff_sign, ~diff_sign, diff_sign);
  assign and_reduce_25217 = &wide_exponent__5[7:0];
  assign not_25248 = ~eq_25241;
  assign or_25249 = ~(p0_result_exponent__2 == MAX_EXPONENT__3 & gt_fraction | p0_result_sign__2) | eq_exp;
  assign p1_enable = p2_stage_done | p2_not_valid;
  assign fraction_shift__8 = 3'h3;
  assign fraction_shift__7 = 3'h4;
  assign has_pos_inf = ~(~eq_24829 | ~eq_24830 | a_sign__1) | ~(~eq_24831 | ~eq_24832 | b_sign__3);
  assign has_neg_inf = eq_24829 & eq_24830 & a_sign__1 | eq_24831 & eq_24832 & b_sign__3;
  assign wide_exponent__2 = wide_exponent__1[8:0] & {9{~wide_exponent__1[9]}};
  assign result_sign__4 = is_operand_inf__1 ? ~has_pos_inf__1 : result_sign__3;
  assign or_25250 = is_result_nan__1 | is_operand_inf__1 | wide_exponent__5[8] | and_reduce_25217;
  assign and_25255 = not_25248 & or_25249;
  assign or_25225 = is_operand_inf__1 | wide_exponent__5[8] | and_reduce_25217 | ~((|wide_exponent__5[8:1]) | wide_exponent__5[0]);
  assign p1_data_enable = p1_enable & p0_valid;
  assign p1_not_valid = ~p0_valid;
  assign rounded_fraction__1 = {rounded_fraction__1_squeezed_portion_3_width_25, normal_chunk__1};
  assign fraction_shift__6 = rounding_carry__1 ? fraction_shift__7 : fraction_shift__8;
  assign fraction_shift__2 = 3'h3;
  assign fraction_shift__4 = 3'h4;
  assign result_sign__5 = ~is_result_nan__1 & result_sign__4;
  assign and_25262 = and_25255 & ~is_result_nan__1;
  assign p0_enable = p1_data_enable | p1_not_valid;
  assign p0_all_active_inputs_valid = __if_add_proc__input_b_valid_reg & __if_add_proc__input_a_valid_reg;
  assign FRACTION_HIGH_BIT__1 = 23'h40_0000;
  assign shrl_25236 = rounded_fraction__1 >> fraction_shift__6;
  assign MAX_EXPONENT__6 = 8'hff;
  assign is_result_nan = ~(~eq_24829 | eq_24830) | ~(~eq_24831 | eq_24832) | has_pos_inf & has_neg_inf;
  assign is_operand_inf = eq_24829 & eq_24830 | eq_24831 & eq_24832;
  assign and_reduce_24858 = &wide_exponent__2[7:0];
  assign rounded_fraction = {rounded_fraction_squeezed_portion_3_width_25, normal_chunk};
  assign fraction_shift__1 = rounding_carry ? fraction_shift__4 : fraction_shift__2;
  assign result_sign = priority_sel_1b_2way({add_24603[25], fraction_is_zero}, a_sign__1 & b_sign__3, ~b_sign__3, b_sign__3);
  assign sel_25259 = or_25249 ? result_sign__5 : ____state_0;
  assign and_25368 = eq_25241 & p1_data_enable;
  assign and_25468 = and_25255 & ~or_25250 & p1_data_enable;
  assign and_25469 = and_25255 & or_25250 & p1_data_enable;
  assign and_25377 = eq_25241 & p1_data_enable;
  assign and_25470 = and_25255 & is_result_nan__1 & p1_data_enable;
  assign and_25471 = and_25262 & ~or_25225 & p1_data_enable;
  assign and_25472 = and_25262 & or_25225 & p1_data_enable;
  assign and_25387 = not_25248 & p1_data_enable;
  assign and_25388 = eq_25241 & p1_data_enable;
  assign add_25264 = {1'h0, ____state_3} + 3'h1;
  assign p0_data_enable = p0_enable & p0_all_active_inputs_valid;
  assign if_add_proc__input_b_valid_inv = ~__if_add_proc__input_b_valid_reg;
  assign if_add_proc__input_a_valid_inv = ~__if_add_proc__input_a_valid_reg;
  assign result_fraction__9 = p1_is_result_nan__1 ? FRACTION_HIGH_BIT__1 : p1_result_fraction__8;
  assign result_fraction__5 = shrl_25236[22:0];
  assign result_exponent__1 = or_25250 ? MAX_EXPONENT__6 : wide_exponent__5[7:0];
  assign MAX_EXPONENT__2 = 8'hff;
  assign shrl_24870 = rounded_fraction >> fraction_shift__1;
  assign result_sign__1 = is_operand_inf ? ~has_pos_inf : result_sign;
  assign concat_25373 = {and_25368, and_25468, and_25469};
  assign MAX_EXPONENT__7 = 8'hff;
  assign F32_ZERO__1_bexp__1 = 8'h00;
  assign concat_25383 = {and_25377, and_25470, and_25471, and_25472};
  assign FRACTION_HIGH_BIT__2 = 23'h40_0000;
  assign concat_25389 = {and_25387, and_25388};
  assign unexpand_for___state_3_next_case_1 = 2'h0;
  assign unexpand_for___state_3_next_case_0 = add_25264[1:0];
  assign if_add_proc__input_b_valid_load_en = p0_data_enable | if_add_proc__input_b_valid_inv;
  assign if_add_proc__input_a_valid_load_en = p0_data_enable | if_add_proc__input_a_valid_inv;
  assign sel_25310 = p1_or_25249 ? result_fraction__9 : p1_____state_2__1;
  assign count__1 = {29'h0000_0000, p1_add_25264};
  assign result_fraction__8 = result_fraction__5 & {23{~or_25225}};
  assign sel_25263 = or_25249 ? result_exponent__1 : ____state_1;
  assign result_exponent__2 = is_result_nan | is_operand_inf | wide_exponent__2[8] | and_reduce_24858 ? MAX_EXPONENT__2 : wide_exponent__2[7:0];
  assign or_24869 = is_operand_inf | wide_exponent__2[8] | and_reduce_24858 | ~((|wide_exponent__2[8:1]) | wide_exponent__2[0]);
  assign result_fraction = shrl_24870[22:0];
  assign result_sign__2 = ~is_result_nan & result_sign__1;
  assign nor_25272 = ~(eq_25241 | ~sel_25259);
  assign one_hot_sel_25374 = MAX_EXPONENT__7 & {8{concat_25373[0]}} | wide_exponent__5[7:0] & {8{concat_25373[1]}} | F32_ZERO__1_bexp__1 & {8{concat_25373[2]}};
  assign or_25375 = and_25368 | and_25468 | and_25469;
  assign one_hot_sel_25384 = 23'h00_0000 & {23{concat_25383[0]}} | result_fraction__5 & {23{concat_25383[1]}} | FRACTION_HIGH_BIT__2 & {23{concat_25383[2]}} | 23'h00_0000 & {23{concat_25383[3]}};
  assign or_25385 = and_25377 | and_25470 | and_25471 | and_25472;
  assign one_hot_sel_25390 = unexpand_for___state_3_next_case_1 & {2{concat_25389[0]}} | unexpand_for___state_3_next_case_0 & {2{concat_25389[1]}};
  assign or_25391 = and_25387 | and_25388;
  assign if_add_proc__input_b_load_en = if_add_proc__input_b_vld & if_add_proc__input_b_valid_load_en;
  assign if_add_proc__input_a_load_en = if_add_proc__input_a_vld & if_add_proc__input_a_valid_load_en;
  assign acc__1 = {p1_sel_25259, p1_sel_25263, sel_25310};
  always @ (posedge clk) begin
    if (reset) begin
      p0_is_result_nan <= 1'h0;
      p0_result_exponent__2 <= 8'h00;
      p0_or_24869 <= 1'h0;
      p0_result_fraction <= 23'h00_0000;
      p0_result_sign__2 <= 1'h0;
      ____state_1 <= 8'h00;
      ____state_2 <= 23'h00_0000;
      ____state_0 <= 1'h0;
      ____state_3 <= 2'h0;
      p1_____state_2__1 <= 23'h00_0000;
      p1_is_result_nan__1 <= 1'h0;
      p1_eq_25241 <= 1'h0;
      p1_result_fraction__8 <= 23'h00_0000;
      p1_or_25249 <= 1'h0;
      p1_not_25248 <= 1'h0;
      p1_sel_25259 <= 1'h0;
      p1_sel_25263 <= 8'h00;
      p1_add_25264 <= 3'h0;
      p0_valid <= 1'h0;
      p1_valid <= 1'h0;
      __if_add_proc__input_b_reg <= __if_add_proc__input_b_reg_init;
      __if_add_proc__input_b_valid_reg <= 1'h0;
      __if_add_proc__input_a_reg <= __if_add_proc__input_a_reg_init;
      __if_add_proc__input_a_valid_reg <= 1'h0;
      __if_add_proc__output_reg <= __if_add_proc__output_reg_init;
      __if_add_proc__output_valid_reg <= 1'h0;
    end else begin
      p0_is_result_nan <= p0_data_enable ? is_result_nan : p0_is_result_nan;
      p0_result_exponent__2 <= p0_data_enable ? result_exponent__2 : p0_result_exponent__2;
      p0_or_24869 <= p0_data_enable ? or_24869 : p0_or_24869;
      p0_result_fraction <= p0_data_enable ? result_fraction : p0_result_fraction;
      p0_result_sign__2 <= p0_data_enable ? result_sign__2 : p0_result_sign__2;
      ____state_1 <= or_25375 ? one_hot_sel_25374 : ____state_1;
      ____state_2 <= or_25385 ? one_hot_sel_25384 : ____state_2;
      ____state_0 <= p1_data_enable ? nor_25272 : ____state_0;
      ____state_3 <= or_25391 ? one_hot_sel_25390 : ____state_3;
      p1_____state_2__1 <= p1_data_enable ? ____state_2 : p1_____state_2__1;
      p1_is_result_nan__1 <= p1_data_enable ? is_result_nan__1 : p1_is_result_nan__1;
      p1_eq_25241 <= p1_data_enable ? eq_25241 : p1_eq_25241;
      p1_result_fraction__8 <= p1_data_enable ? result_fraction__8 : p1_result_fraction__8;
      p1_or_25249 <= p1_data_enable ? or_25249 : p1_or_25249;
      p1_not_25248 <= p1_data_enable ? not_25248 : p1_not_25248;
      p1_sel_25259 <= p1_data_enable ? sel_25259 : p1_sel_25259;
      p1_sel_25263 <= p1_data_enable ? sel_25263 : p1_sel_25263;
      p1_add_25264 <= p1_data_enable ? add_25264 : p1_add_25264;
      p0_valid <= p0_enable ? p0_all_active_inputs_valid : p0_valid;
      p1_valid <= p1_enable ? p0_valid : p1_valid;
      __if_add_proc__input_b_reg <= if_add_proc__input_b_load_en ? if_add_proc__input_b : __if_add_proc__input_b_reg;
      __if_add_proc__input_b_valid_reg <= if_add_proc__input_b_valid_load_en ? if_add_proc__input_b_vld : __if_add_proc__input_b_valid_reg;
      __if_add_proc__input_a_reg <= if_add_proc__input_a_load_en ? if_add_proc__input_a : __if_add_proc__input_a_reg;
      __if_add_proc__input_a_valid_reg <= if_add_proc__input_a_valid_load_en ? if_add_proc__input_a_vld : __if_add_proc__input_a_valid_reg;
      __if_add_proc__output_reg <= if_add_proc__output_load_en ? acc__1 : __if_add_proc__output_reg;
      __if_add_proc__output_valid_reg <= if_add_proc__output_valid_load_en ? __if_add_proc__output_vld_buf : __if_add_proc__output_valid_reg;
    end
  end
  assign if_add_proc__output = __if_add_proc__output_reg;
  assign if_add_proc__output_vld = __if_add_proc__output_valid_reg;
  assign if_add_proc__input_b_rdy = if_add_proc__input_b_load_en;
  assign if_add_proc__input_a_rdy = if_add_proc__input_a_load_en;
  `ifdef SIMULATION
  always @ (posedge clk) begin
    if (p0_all_active_inputs_valid) begin
      $display("[DUT]: Got (APFloat{sign: %d, bexp: %d, fraction: %d},APFloat{sign: %d, bexp: %d, fraction: %d})", tuple_index_24577, a_bexp__2, tuple_index_24553, b_sign__2, b_bexp__4, b_fraction__4);
    end
  end
  `endif
  `ifdef SIMULATION
  always @ (posedge clk) begin
    if (p2_stage_done & p1_eq_25241) begin
      $display("[DUT]: Done. Sending APFloat{sign: %d, bexp: %d, fraction: %d}", p1_sel_25259, p1_sel_25263, sel_25310);
    end
  end
  `endif
  `ifdef SIMULATION
  always @ (posedge clk) begin
    if (p2_stage_done & p1_not_25248) begin
      $display("[DUT]: Counting. count: %d, acc: APFloat{sign: %d, bexp: %d, fraction: %d}", count__1, p1_sel_25259, p1_sel_25263, sel_25310);
    end
  end
  `endif
endmodule


module __if_add_proc__if_add_proc_wrapper_0_next(
  input wire clk,
  input wire reset,
  input wire [31:0] if_add_proc__input_a,
  input wire if_add_proc__input_a_vld,
  input wire [31:0] if_add_proc__input_b,
  input wire if_add_proc__input_b_vld,
  input wire if_add_proc__output_rdy,
  output wire if_add_proc__input_a_rdy,
  output wire if_add_proc__input_b_rdy,
  output wire [31:0] if_add_proc__output,
  output wire if_add_proc__output_vld
);
  wire instantiation_output_25436;
  wire instantiation_output_25442;
  wire [31:0] instantiation_output_25446;
  wire instantiation_output_25447;

  // ===== Instantiations
  __if_add_proc__if_add_proc_wrapper_0_next__1 __if_add_proc__if_add_proc_wrapper_0_next__1_inst0 (
    .reset(reset),
    .clk(clk)
  );
  __if_add_proc__if_add_proc_wrapper__if_add_proc_0__3_next __if_add_proc__if_add_proc_wrapper__if_add_proc_0__3_next_inst1 (
    .reset(reset),
    .if_add_proc__input_a(if_add_proc__input_a),
    .if_add_proc__input_a_vld(if_add_proc__input_a_vld),
    .if_add_proc__input_b(if_add_proc__input_b),
    .if_add_proc__input_b_vld(if_add_proc__input_b_vld),
    .if_add_proc__output_rdy(if_add_proc__output_rdy),
    .if_add_proc__input_a_rdy(instantiation_output_25436),
    .if_add_proc__input_b_rdy(instantiation_output_25442),
    .if_add_proc__output(instantiation_output_25446),
    .if_add_proc__output_vld(instantiation_output_25447),
    .clk(clk)
  );
  assign if_add_proc__input_a_rdy = instantiation_output_25436;
  assign if_add_proc__input_b_rdy = instantiation_output_25442;
  assign if_add_proc__output = instantiation_output_25446;
  assign if_add_proc__output_vld = instantiation_output_25447;
endmodule


module testbench;
  reg clk;
  reg reset;
  reg [31:0] if_add_proc__input_a;
  reg if_add_proc__input_a_vld;
  reg [31:0] if_add_proc__input_b;
  reg if_add_proc__input_b_vld;
  reg if_add_proc__output_rdy;
  wire if_add_proc__input_a_rdy;
  wire if_add_proc__input_b_rdy;
  wire [31:0] if_add_proc__output;
  wire if_add_proc__output_vld;
  __if_add_proc__if_add_proc_wrapper_0_next dut (
    .clk(clk),
    .reset(reset),
    .if_add_proc__input_a(if_add_proc__input_a),
    .if_add_proc__input_a_vld(if_add_proc__input_a_vld),
    .if_add_proc__input_b(if_add_proc__input_b),
    .if_add_proc__input_b_vld(if_add_proc__input_b_vld),
    .if_add_proc__output_rdy(if_add_proc__output_rdy),
    .if_add_proc__input_a_rdy(if_add_proc__input_a_rdy),
    .if_add_proc__input_b_rdy(if_add_proc__input_b_rdy),
    .if_add_proc__output(if_add_proc__output),
    .if_add_proc__output_vld(if_add_proc__output_vld)
  );

  // Clock generator.
  initial begin
    clk = 0;
    forever #5 clk = !clk;
  end

  // Monitor for input/output ports.
  initial begin
    $dumpfile("testbench.vcd");
    $dumpvars(0,testbench);
    $display("Clock rises at 5, 15, 25, ....");
    $display("Signals driven one time unit after rising clock.");
    $display("Signals sampled one time unit before rising clock.");
    $display("Starting simulation. Monitor output:");
    $monitor("%t reset: %d if_add_proc__input_a: %d if_add_proc__input_a_vld: %d if_add_proc__input_b: %d if_add_proc__input_b_vld: %d if_add_proc__output_rdy: %d if_add_proc__input_a_rdy: %d if_add_proc__input_b_rdy: %d if_add_proc__output: %d if_add_proc__output_vld: %d", $time, reset, if_add_proc__input_a, if_add_proc__input_a_vld, if_add_proc__input_b, if_add_proc__input_b_vld, if_add_proc__output_rdy, if_add_proc__input_a_rdy, if_add_proc__input_b_rdy, if_add_proc__output, if_add_proc__output_vld);
  end

  // Thread `reset_controller`. Drives signals: reset
  reg __last_cycle_of_reset;
  initial begin
    reset = 1'h1;
    __last_cycle_of_reset = 1'h0;

    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait 4 cycle(s).
    repeat (3) begin
      @(posedge clk);
    end
    #1;
    @(posedge clk);
    #1;
    __last_cycle_of_reset = 1'h1;
    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    __last_cycle_of_reset = 1'h0;
    reset = 1'h0;
  end

  // Thread `watchdog`. Drives signals: <none>
  initial begin
    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait 10005 cycle(s).
    repeat (10004) begin
      @(posedge clk);
    end
    #1;
    @(posedge clk);
    #1;
    $display("ERROR: timeout, simulation ran too long (10000 cycles).");
    $finish;
  end

  // Thread `if_add_proc__input_a driver`. Drives signals: if_add_proc__input_a, if_add_proc__input_a_vld
  initial begin
    if_add_proc__input_a = 32'dx;
    if_add_proc__input_a_vld = 1'h0;

    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait for last cycle of reset
    #8;
    while (!(1'h1 && __last_cycle_of_reset == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_a = 32'h3f80_0000;
    if_add_proc__input_a_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_a_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_a_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_a = 32'h3f80_0000;
    if_add_proc__input_a_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_a_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_a_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_a = 32'h3f80_0000;
    if_add_proc__input_a_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_a_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_a_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_a = 32'h3f80_0000;
    if_add_proc__input_a_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_a_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_a_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_a_vld = 1'h0;
    if_add_proc__input_a = 32'dx;
  end

  // Thread `if_add_proc__input_b driver`. Drives signals: if_add_proc__input_b, if_add_proc__input_b_vld
  initial begin
    if_add_proc__input_b = 32'dx;
    if_add_proc__input_b_vld = 1'h0;

    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait for last cycle of reset
    #8;
    while (!(1'h1 && __last_cycle_of_reset == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_b = 32'h0000_0000;
    if_add_proc__input_b_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_b_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_b_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_b = 32'h0000_0000;
    if_add_proc__input_b_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_b_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_b_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_b = 32'h0000_0000;
    if_add_proc__input_b_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_b_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_b_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_b = 32'h0000_0000;
    if_add_proc__input_b_vld = 1'h1;
    // Wait for cycle after `if_add_proc__input_b_rdy` is asserted
    #8;
    while (!(1'h1 && if_add_proc__input_b_rdy == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__input_b_vld = 1'h0;
    if_add_proc__input_b = 32'dx;
  end

  // Thread `if_add_proc__output driver`. Drives signals: if_add_proc__output_rdy
  initial begin
    if_add_proc__output_rdy = 1'h0;

    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait for last cycle of reset
    #8;
    while (!(1'h1 && __last_cycle_of_reset == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    if_add_proc__output_rdy = 1'h1;
  end

  // Thread `output if_add_proc__output capture`. Drives signals: <none>
  reg __thread_output_if_add_proc__output_capture_done;
  initial begin
    __thread_output_if_add_proc__output_capture_done = 1'h0;

    // Wait 1 cycle(s).
    @(posedge clk);
    #1;
    // Wait for last cycle of reset
    #8;
    while (!(1'h1 && __last_cycle_of_reset == 1'h1)) begin
      #10;
    end
    @(posedge clk);
    #1;
    // Wait for all asserted (and capture output): if_add_proc__output_vld, if_add_proc__output_rdy
    #8;
    while (!(1'h1 && if_add_proc__output_vld == 1'h1 && if_add_proc__output_rdy == 1'h1)) begin
      #10;
    end
    $display("%t OUTPUT if_add_proc__output = 32'h%0x (#0)", $time, if_add_proc__output);
    @(posedge clk);
    #1;

    __thread_output_if_add_proc__output_capture_done = 1'h1;
  end

  // Thread completion monitor.
  initial begin
    @(posedge clk);
    #9;
    while (!__thread_output_if_add_proc__output_capture_done) begin
      @(posedge clk);
      #9;
    end
    @(posedge clk);
    #1;
    $finish;
  end
endmodule

